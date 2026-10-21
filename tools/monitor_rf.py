#!/usr/bin/env python3
"""Record read-only CC1101 diagnostics and native packets through one USB port.

Read-only by default. --margin-db explicitly changes the RAM-only capture margin.
Never replays data or changes credentials, services, or on-device history.
"""
import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import time

from firmware_console import DEFAULT_PORT, response, send, synchronize


def record(stream, item):
    if item.get('event') not in ('rf_status', 'rf_packet', 'rf_monitor', 'rf_margin', 'boot', 'fatal'):
        return
    entry = dict(host_utc=datetime.now(timezone.utc).isoformat(),
                 host_monotonic_ns=time.monotonic_ns(), device=item)
    stream.write(json.dumps(entry, separators=(',', ':'), allow_nan=False)+'\n')
    stream.flush()


def apply_margin(port, stream, margin):
    if not isinstance(margin, int) or not 3 <= margin <= 15:
        raise ValueError('Capture margin must be an integer from 3 to 15 dB')
    for attempt in range(3):
        rid = send(port, dict(op='rf_margin', margin_db=margin))
        try:
            reply = response(port, 'rf_margin', request_id=rid)
            record(stream, reply)
            return reply
        except TimeoutError:
            if attempt == 2:
                raise



def monitor(port, stream, seconds, interval):
    rid = send(port, dict(op='rf_monitor', enabled=True))
    reply = response(port, 'rf_monitor', request_id=rid)
    record(stream, reply)
    until = time.monotonic()+seconds
    next_poll = 0
    counts = dict(packets=0, snapshots=0, malformed=0)
    try:
        while time.monotonic() < until:
            now = time.monotonic()
            if now >= next_poll:
                send(port, dict(op='rf_status'))
                next_poll = now+interval
            line = port.readline()
            if not line:
                continue
            try:
                item = json.loads(line)
            except (ValueError, UnicodeDecodeError):
                counts['malformed'] += 1
                continue
            record(stream, item)
            if item.get('event') in ('error', 'fatal'):
                raise RuntimeError(item)
            if item.get('event') == 'rf_packet':
                counts['packets'] += 1
            elif item.get('event') == 'rf_status':
                counts['snapshots'] += 1
    finally:
        # No weather/model reset. If USB disappears, firmware stops the stream
        # automatically when its 15-second host-heartbeat lease expires.
        try:
            send(port, dict(op='rf_monitor', enabled=False))
        except OSError:
            pass
    return counts


def main():
    import serial
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--port', default=DEFAULT_PORT)
    parser.add_argument('--seconds', type=float, default=300)
    parser.add_argument('--interval', type=float, default=1)
    parser.add_argument('--margin-db', type=int, choices=range(3, 16), help='Explicit temporary capture-margin override; resets on device restart')
    args = parser.parse_args()
    if not math.isfinite(args.seconds) or args.seconds <= 0:
        parser.error('--seconds must be positive and finite')
    if not math.isfinite(args.interval) or not .2 <= args.interval <= 5:
        parser.error('--interval must be between 0.2 and 5 seconds')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    port = serial.Serial(port=None, baudrate=115200, timeout=.25,
                         write_timeout=5, exclusive=True)
    port.dtr = True
    port.rts = True
    port.port = args.port
    with port:
        status = synchronize(port)
        if status.get('source') in ('usb_replay', 'replay'):
            raise RuntimeError('Live RF monitoring needs the device out of replay mode')
        print('Connected to firmware', status.get('version'), flush=True)
        with args.output.open('x', encoding='utf-8') as stream:
            if args.margin_db is not None:
                print('Temporary margin:', apply_margin(port, stream, args.margin_db), flush=True)
            try:
                result = monitor(port, stream, args.seconds, args.interval)
            except KeyboardInterrupt:
                print('Stopped; captured data retained in', args.output)
                return
    print(json.dumps(dict(output=str(args.output), **result)))


if __name__ == '__main__':
    main()
