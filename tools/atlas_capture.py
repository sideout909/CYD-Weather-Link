#!/usr/bin/env python3
"""Continuously archive decoded rtl_433 packets, with reconnects and UTC rotation."""

import argparse
from collections import Counter
from datetime import datetime, timezone
import fcntl
import json
import logging
from logging.handlers import RotatingFileHandler
import os
from pathlib import Path
import selectors
import shutil
import signal
import subprocess
import time
import uuid


def utc_now():
    return datetime.now(timezone.utc).isoformat(timespec="microseconds")


def write_json(path, value):
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def clock_synchronized():
    try:
        result = subprocess.run(
            ["timedatectl", "show", "--value", "-p", "NTPSynchronized"],
            capture_output=True, text=True, timeout=2, check=True,
        )
        return result.stdout.strip() == "yes"
    except (OSError, subprocess.SubprocessError):
        return None


class Archive:
    def __init__(self, output):
        self.output = output
        output.mkdir(parents=True, exist_ok=True)
        self.session = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ-") + uuid.uuid4().hex[:8]
        self.file = None
        self.day = None
        self.counts = Counter()
        self.state = {
            "session": self.session, "started_utc": utc_now(), "state": "starting",
            "packets_this_session": 0, "receiver_starts": 0,
            "last_packet_utc": None, "last_atlas_packet_utc": None,
            "last_lux_utc": None, "last_lux": None,
            "clock_synchronized": clock_synchronized(),
        }

    def append(self, packet, received_utc=None, monotonic_ns=None):
        stamp = received_utc or utc_now()
        if stamp[:10] != self.day:
            self.close()
            path = self.output / f"packets-{stamp[:10]}.jsonl"
            incomplete = False
            if path.exists() and path.stat().st_size:
                with path.open("rb") as existing:
                    existing.seek(-1, os.SEEK_END)
                    incomplete = existing.read(1) != b"\n"
            self.file = path.open("a", buffering=1, encoding="utf-8")
            if incomplete:
                self.file.write("\n")
            self.day = stamp[:10]
        sequence = self.state["packets_this_session"] + 1
        record = dict(packet)
        record["_capture"] = {
            "session": self.session, "sequence": sequence, "received_utc": stamp,
            "received_monotonic_ns": monotonic_ns if monotonic_ns is not None else time.monotonic_ns(),
            "clock_synchronized": self.state["clock_synchronized"],
        }
        self.file.write(json.dumps(record, separators=(",", ":"), allow_nan=False) + "\n")
        self.state["packets_this_session"] = sequence
        self.state["last_packet_utc"] = stamp
        self.counts[str(packet.get("message_type", "other"))] += 1
        if (packet.get("model"), packet.get("id"), packet.get("channel")) == ("Acurite-Atlas", 679, "A"):
            self.state["last_atlas_packet_utc"] = stamp
            if "lux" in packet:
                self.state["last_lux"] = packet["lux"]
                self.state["last_lux_utc"] = stamp

    def checkpoint(self):
        if self.file:
            self.file.flush()
            os.fsync(self.file.fileno())
        self.state["updated_utc"] = utc_now()
        self.state["packets_by_message_type_this_session"] = dict(self.counts)
        write_json(self.output / "status.json", self.state)

    def close(self):
        if self.file:
            self.file.flush()
            os.fsync(self.file.fileno())
            self.file.close()
            self.file = None


def receive(command, archive, logger, stopping, min_free_bytes):
    """Drain both pipes, checkpoint even with no packets, and allow clean stops."""
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    archive.state["receiver_starts"] += 1
    archive.state["receiver_pid"] = process.pid
    archive.state["state"] = "listening"
    logger.info("Starting receiver pid=%s", process.pid)
    selector = selectors.DefaultSelector()
    buffers = {}
    for stream, name in ((process.stdout, "packets"), (process.stderr, "diagnostics")):
        selector.register(stream, selectors.EVENT_READ, name)
        buffers[name] = b""
    next_checkpoint = 0
    next_clock = time.monotonic() + 60
    try:
        while selector.get_map() and not stopping[0]:
            now = time.monotonic()
            if now >= next_checkpoint:
                free_bytes = shutil.disk_usage(archive.output).free
                archive.state["free_disk_bytes"] = free_bytes
                if free_bytes < min_free_bytes:
                    raise OSError("Free disk space is below the configured reserve; retained all existing logs")
                archive.checkpoint()
                next_checkpoint = now + 15
            if now >= next_clock:
                archive.state["clock_synchronized"] = clock_synchronized()
                next_clock = now + 60
            for key, _ in selector.select(timeout=1):
                chunk = os.read(key.fd, 65536)
                name = key.data
                if not chunk:
                    selector.unregister(key.fileobj)
                    if buffers[name]:
                        logger.warning("Incomplete final %s line discarded", name)
                    continue
                buffers[name] += chunk
                if len(buffers[name]) > 1024 * 1024:
                    raise ValueError("Receiver output exceeded the maximum line buffer")
                while b"\n" in buffers[name]:
                    line, buffers[name] = buffers[name].split(b"\n", 1)
                    if name == "diagnostics":
                        logger.info("rtl_433: %s", line.decode("utf-8", errors="replace"))
                        continue
                    if not line.lstrip().startswith(b"{"):
                        logger.info("rtl_433: %s", line.decode("utf-8", errors="replace"))
                        continue
                    try:
                        packet = json.loads(line)
                        if not isinstance(packet, dict) or "model" not in packet:
                            raise ValueError("Not a decoded device packet")
                        archive.append(packet)
                    except (ValueError, UnicodeError) as error:
                        logger.warning("Rejected invalid packet output: %s", error)
        return process.poll()
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        selector.close()
        process.stdout.close()
        process.stderr.close()
        archive.state["receiver_pid"] = None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--device", default=":00000001")
    parser.add_argument("--min-free-mib", type=int, default=256)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    capture_lock = (args.output / ".capture.lock").open("a")
    try:
        fcntl.flock(capture_lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        parser.exit(78, "Another capture is already writing to this directory.\n")
    logger = logging.getLogger("atlas-capture")
    logger.setLevel(logging.INFO)
    handler = RotatingFileHandler(args.output / "receiver.log", maxBytes=2 * 1024 * 1024, backupCount=3)
    handler.setFormatter(logging.Formatter("%(asctime)sZ %(levelname)s %(message)s"))
    handler.formatter.converter = time.gmtime
    logger.addHandler(handler)
    archive = Archive(args.output)
    stopping = [False]
    for signum in (signal.SIGTERM, signal.SIGINT):
        signal.signal(signum, lambda *_: stopping.__setitem__(0, True))
    command = [
        "/usr/bin/stdbuf", "-oL", "-eL", "/usr/bin/rtl_433", "-c", "/dev/null", "-d", args.device,
        "-f", "433.92M", "-s", "250k", "-R", "40",
        "-M", "time:iso:usec:utc", "-M", "level", "-M", "protocol",
        "-F", "log", "-F", "json",
    ]
    context_path = args.output / "context.json"
    context = json.loads(context_path.read_text()) if context_path.exists() else {}
    write_json(args.output / f"session-{archive.session}.json", {
        "session": archive.session, "started_utc": archive.state["started_utc"],
        "command": command, "context": context,
        "package_versions": subprocess.run(["pacman", "-Q", "rtl-sdr", "rtl_433"], capture_output=True, text=True).stdout.strip(),
        "python_version": os.sys.version,
    })
    logger.info("Capture session %s started; no time limit", archive.session)
    exit_code = 0
    try:
        while not stopping[0]:
            try:
                code = receive(command, archive, logger, stopping, args.min_free_mib * 1024 * 1024)
                if not stopping[0]:
                    logger.warning("Receiver exited with code %s; retry in 5 seconds", code)
            except ValueError as error:
                logger.error("Receiver stream error: %s; retry in 5 seconds", error)
            if not stopping[0]:
                archive.state["state"] = "retrying"
                archive.checkpoint()
                for _ in range(50):
                    if stopping[0]:
                        break
                    time.sleep(0.1)
    except OSError as error:
        logger.error("Capture stopped: %s", error)
        archive.state["error"] = str(error)
        exit_code = 78
    finally:
        archive.state["state"] = "error" if exit_code else "stopped"
        archive.state["stopped_utc"] = utc_now()
        try:
            archive.checkpoint()
            archive.close()
        except OSError as error:
            logger.error("Final disk flush failed: %s", error)
            exit_code = 78
        logger.info("Capture ended with code %s", exit_code)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
