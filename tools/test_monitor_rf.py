import io
import json
import unittest
from unittest.mock import patch

import monitor_rf


class Port:
    def __init__(self, lines):
        self.lines = iter(lines)

    def readline(self):
        return next(self.lines, b'')


class MonitorTests(unittest.TestCase):
    def test_explicit_margin_override_is_bounded_and_logged(self):
        output=io.StringIO()
        with patch.object(monitor_rf, 'send', return_value=7) as send, \
             patch.object(monitor_rf, 'response', return_value=dict(event='rf_margin', margin_db=6)):
            monitor_rf.apply_margin(object(), output, 6)
            self.assertEqual(send.call_args.args[1], dict(op='rf_margin', margin_db=6))
            self.assertEqual(json.loads(output.getvalue())['device']['margin_db'], 6)
            for bad in (0, 2, 16, 6.5):
                with self.assertRaises(ValueError):
                    monitor_rf.apply_margin(object(), output, bad)
            self.assertEqual(send.call_count, 1)

    def test_capture_ignores_unrelated_output_and_uses_only_monitor_commands(self):
        port = Port([b'{"event":"rf_packet","packet":{"rssi":-83}}\n',
                     b'broken\n', b'{"event":"rf_status","threshold_dbm":-88}\n',
                     b'{"event":"config","private":"not-recorded"}\n'])
        output = io.StringIO()
        with patch.object(monitor_rf, 'send', return_value=1) as send, \
             patch.object(monitor_rf, 'response', return_value=dict(event='rf_monitor', enabled=True)), \
             patch.object(monitor_rf.time, 'monotonic', side_effect=iter(range(100))):
            result = monitor_rf.monitor(port, output, 12, 1)
        self.assertEqual(result['packets'], 1)
        self.assertEqual(result['snapshots'], 1)
        self.assertEqual(result['malformed'], 1)
        self.assertNotIn('not-recorded', output.getvalue())
        records = [json.loads(line) for line in output.getvalue().splitlines()]
        self.assertTrue(all('host_monotonic_ns' in r for r in records))
        commands = [call.args[1] for call in send.call_args_list]
        self.assertEqual(commands[0], dict(op='rf_monitor', enabled=True))
        self.assertEqual(commands[-1], dict(op='rf_monitor', enabled=False))
        self.assertTrue(all(c['op'] in ('rf_monitor', 'rf_status') for c in commands))

    def test_device_error_still_disables_stream(self):
        port = Port([b'{"event":"fatal","reason":"synthetic"}\n'])
        with patch.object(monitor_rf, 'send', return_value=1) as send, \
             patch.object(monitor_rf, 'response', return_value=dict(event='rf_monitor')), \
             patch.object(monitor_rf.time, 'monotonic', side_effect=iter(range(100))):
            with self.assertRaises(RuntimeError):
                monitor_rf.monitor(port, io.StringIO(), 10, 1)
        self.assertEqual(send.call_args.args[1], dict(op='rf_monitor', enabled=False))

    def test_lost_margin_acknowledgement_can_retry_idempotently(self):
        output=io.StringIO()
        with patch.object(monitor_rf, 'send', side_effect=[7,8]) as send, \
             patch.object(monitor_rf, 'response', side_effect=[TimeoutError(), dict(event='rf_margin', margin_db=6)]):
            reply=monitor_rf.apply_margin(object(), output, 6)
        self.assertEqual(reply['margin_db'],6)
        self.assertEqual(send.call_count,2)
        self.assertEqual(len(output.getvalue().splitlines()),1)


if __name__ == '__main__':
    unittest.main()
