import unittest
from firmware_console import sample_command, response, screenshot
from pathlib import Path


class ReplayAdapterTests(unittest.TestCase):
    def packet(self, **extra):
        return dict(model='Acurite-Atlas', id=679, channel='A',
                    message_type=39, time='2026-10-18T00:00:00', **extra)

    def test_sparse_zero_values_and_explicit_utc(self):
        result = sample_command(self.packet(lux=0, uvi=0))
        self.assertEqual(result['lux'], 0)
        self.assertEqual(result['uv'], 0)
        self.assertNotIn('temperature_F', result)
        self.assertEqual(result['time_ms'], 1792281600000)

    def test_flags_rain_and_station_filter(self):
        result = sample_command(self.packet(exception=1, rain_in=5.11))
        self.assertTrue(result['exception'])
        self.assertEqual(result['rain_tips'], 511)
        other = self.packet(); other['id'] = 680
        self.assertIsNone(sample_command(other))

    def test_screenshot_refuses_setup_access_key_view(self):
        class Port:
            def write(self, data): return len(data)
            def readline(self): return b'{"event":"error","reason":"setup_view_contains_key"}\n'
        with self.assertRaisesRegex(RuntimeError, 'setup_view_contains_key'):
            screenshot(Port(), Path('/tmp/unused-weather-screen.png'))

    def test_bad_time_is_not_replaced_with_current_time(self):
        invalid = self.packet(); invalid['time'] = 'not a timestamp'
        with self.assertRaises(ValueError): sample_command(invalid)

    def test_response_does_not_confuse_periodic_diagnostics_with_reply(self):
        class Port:
            def __init__(self):
                self.lines = iter([
                    b'{"event":"status","request_id":0,"accepted":1}\n',
                    b'{"event":"status","request_id":25,"accepted":2}\n'])
            def readline(self): return next(self.lines)
        self.assertEqual(response(Port(), 'status', request_id=25)['accepted'], 2)


if __name__ == '__main__': unittest.main()
