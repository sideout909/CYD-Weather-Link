"""Archive integrity and receiver lifecycle checks, using synthetic packets."""

import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import Mock, patch

spec = importlib.util.spec_from_file_location("atlas_capture", Path(__file__).with_name("atlas_capture.py"))
capture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(capture)


class CaptureTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        with patch.object(capture, "clock_synchronized", return_value=True):
            self.archive = capture.Archive(Path(self.temporary.name))
        self.addCleanup(self.archive.close)

    def packet(self, **fields):
        return {"model": "Acurite-Atlas", "id": 679, "channel": "A", **fields}

    def test_repeats_missing_fields_and_zero_lux_are_preserved(self):
        for sequence in range(3):
            self.archive.append(self.packet(message_type=39, sequence_num=sequence, lux=0))
        self.archive.append(self.packet(message_type=37, temperature_F=62.4))
        rows = [json.loads(line) for line in next(self.archive.output.glob("packets-*.jsonl")).read_text().splitlines()]
        self.assertEqual(len(rows), 4)
        self.assertEqual([r["sequence_num"] for r in rows[:3]], [0, 1, 2])
        self.assertNotIn("lux", rows[3])
        self.assertEqual(self.archive.state["last_lux"], 0)
        self.assertEqual([r["_capture"]["sequence"] for r in rows], [1, 2, 3, 4])

    def test_utc_rotation_and_append_on_new_session(self):
        self.archive.append(self.packet(lux=123), "2026-10-16T23:59:59+00:00", 100)
        self.archive.append(self.packet(lux=456), "2026-10-17T00:00:01+00:00", 200)
        self.archive.close()
        with patch.object(capture, "clock_synchronized", return_value=False):
            second = capture.Archive(self.archive.output)
        try:
            second.append(self.packet(lux=789), "2026-10-17T00:00:02+00:00", 300)
        finally:
            second.close()
        rows = [json.loads(line) for line in (self.archive.output / "packets-2026-10-17.jsonl").read_text().splitlines()]
        self.assertEqual([row["lux"] for row in rows], [456, 789])
        self.assertNotEqual(rows[0]["_capture"]["session"], rows[1]["_capture"]["session"])
        self.assertFalse(rows[1]["_capture"]["clock_synchronized"])

    def test_other_station_is_logged_without_changing_target_lux(self):
        self.archive.append(self.packet(lux=5))
        self.archive.append(self.packet(id=999, lux=900))
        self.assertEqual(self.archive.state["packets_this_session"], 2)
        self.assertEqual(self.archive.state["last_lux"], 5)

    def test_append_after_partial_power_loss_line_keeps_next_packet_readable(self):
        path = self.archive.output / "packets-2026-10-16.jsonl"
        path.write_text('{"model":"Acurite-Atlas","lux":')
        self.archive.append(self.packet(lux=321), "2026-10-16T05:00:00+00:00")
        lines = path.read_text().splitlines()
        self.assertEqual(len(lines), 2)
        self.assertEqual(json.loads(lines[1])["lux"], 321)

    def test_receiver_exit_drains_packets_and_separates_diagnostics(self):
        command = [sys.executable, "-c", 'import sys; print(\'{"model":"Acurite-Atlas","id":679,"channel":"A","lux":123}\'); print("diagnostic", file=sys.stderr)']
        result = capture.receive(command, self.archive, Mock(), [False], 0)
        self.assertIn(result, (0, None))
        self.assertEqual(self.archive.state["packets_this_session"], 1)
        self.assertEqual(self.archive.state["last_lux"], 123)
        self.assertIsNone(self.archive.state["receiver_pid"])

    def test_silent_receiver_can_be_stopped(self):
        stopping = [False]
        timer = threading.Timer(0.2, lambda: stopping.__setitem__(0, True))
        timer.start()
        self.addCleanup(timer.cancel)
        capture.receive([sys.executable, "-c", "import time; time.sleep(30)"], self.archive, Mock(), stopping, 0)
        self.assertIsNone(self.archive.state["receiver_pid"])

    def test_low_disk_stops_without_deleting_existing_packets(self):
        self.archive.append(self.packet(lux=23))
        command = [sys.executable, "-c", "import time; time.sleep(30)"]
        with self.assertRaises(OSError):
            capture.receive(command, self.archive, Mock(), [False], 10**30)
        rows = list(self.archive.output.glob("packets-*.jsonl"))
        self.assertEqual(len(rows), 1)
        self.assertIn('"lux":23', rows[0].read_text())


if __name__ == "__main__":
    unittest.main()
