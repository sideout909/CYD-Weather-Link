# Atlas reception diagnostics

## Expected timing and the saved overnight baseline

[AcuRite's Atlas specifications](https://www.acurite.com/products/acurite-atlas-weather-sensor)
state transmissions every 10 seconds; wind reports every 10 seconds and the
other weather fields every 30 seconds. Calm/dark/unchanged conditions alone
do not make a received measurement stale. The firmware accepts a new matching
payload after the three-second same-burst duplicate window, even if its values
have not changed. A complete, fresh packet cycle is still required for uploads.

The October 21 review of 57,687 saved SDR packets found:

| Captured night, local EDT | Any-type maximum gap | Usable single-type maximum gap |
| --- | ---: | ---: |
| Oct 16, 01:33–08:00 (partial) | 20.975 s | 92.794 s |
| Oct 17, 20:35–Oct 18, 08:00 (partial) | 20.971 s | 92.284 s |
| Oct 18, 20:00–Oct 19, 08:00 | 20.975 s | 92.280 s |

No measured overnight 37/38/39 gap reached 120 seconds. The 26.5-hour break
between October 16 and 17 was a stopped recording session, confirmed by receiver
logs; it is not evidence of a silent sensor. Night windows are 20:00–08:00,
not calculated sunset/sunrise. Median type-specific spacing was about 30.93 s.
One steady 39.6 °F / 74% RH interval still produced eleven type-37 transmissions
over 307 seconds. The private reproducible review and input hashes are in
`captures/atlas/reviews/overnight-20261021/`. Raw captures remain ignored.

This is historical SDR evidence, not a measurement of today's CC1101 link.
The current antenna, power, interference, receiver settings, decoding and
transmitter operation still need live diagnosis.

## Read-only USB monitoring, v0.4.3 candidate

```bash
bash tools/firmware-console.sh checkpoint-save
bash tools/monitor-rf.sh captures/rf-baseline.jsonl --seconds 600
```

Requires the diagnostics firmware. Do not run another serial command while the
monitor owns the port. Capture one baseline without moving the antenna or
changing settings; document any unavoidable move/power-source change. The
collector creates a new JSONL file exclusively, flushing each record. It does
not replay packets, reset weather, start the old SDR service, or modify tuning.

- `rf_status` requests once per second report current RSSI, the library's average
  RSSI/background estimate, adaptive threshold, decoder attempts/messages/
  zero-decode attempts, capture/decoder/application queue drops, type ages
  (array order 37/38/39), field ages and current station health.
- Opt-in `rf_packet` records contain the native weather JSON, including its
  capture-start RSSI, application ingest result and uptime. Result values:
  0 accepted, 1 same-burst duplicate, 2 wrong station, 3 sensor exception,
  4 invalid, 5 backwards timestamp. Missing fields remain absent.
- The host adds UTC and monotonic receipt timestamps. Device uptimes identify
  restarts; counters restart with firmware. These records contain RF weather
  diagnostics, not saved credentials. Partial recordings remain usable.
- Streaming stops on exit or after a 15-second host-heartbeat timeout. It is
  disabled by default and never writes packet logs to device flash. Normal
  history and upload freshness gates continue unchanged.

The average RSSI includes what the receiver samples; it is a background
estimate, not calibrated laboratory noise power. A decoded packet's RSSI minus
that estimate is a useful relative margin, not a certified SNR. RSSI for a
packet that never crosses the capture threshold is unavailable. A strong
interferer can also raise RSSI; decode failures and per-type gaps must be
considered alongside signal level. SDR RSSI units are not interchangeable with
CC1101 dBm estimates. No reception diagnosis follows from RSSI alone.

## Receiver settings available for later controlled trials

The [pinned receiver library](https://github.com/NorthernMan54/rtl_433_ESP/blob/f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8/src/rtl_433_ESP.h)
currently supplies these defaults, with no application override:

| Setting | Current baseline | Possible experiment |
| --- | --- | --- |
| Center frequency | 433.92 MHz, application-selected | Verify actual transmitter offset against a reference before narrowing bandwidth. SDR frequency estimates without calibration are not exact carrier measurements. |
| CC1101 receive filter | `CC1101_RX_BANDWIDTH=812.0f`, selects 812.5 kHz | Try a narrower supported filter, such as 270.8 or 203.1 kHz, while preserving the transmitter's occupied signal and drift. |
| Adaptive capture margin | `RSSI_THRESHOLD=9` dB above average | Compare a modest lower margin, e.g. 6 dB, with baseline. More noise pulses/failed decodes may outweigh additional weak captures. |
| AGC registers | `0xC7 / 0x40 / 0xB2` | Evaluate only after baseline and frequency/bandwidth trials; do not assume maximum gain is best. |

Narrowing a properly centered filter can reduce admitted noise, but can also
exclude an offset signal. Reducing the software capture margin does not amplify
RF. The library's `setOOKThreshold` helper is for SX127x, not this CC1101.
TI's [OOK/ASK tuning note DN022](https://www.ti.com/lit/an/swra215e/swra215e.pdf)
explains that optimal AGC settings depend on data rate/bandwidth and must be
checked across input levels for unstable reception.

Judge trials by separate bound-station transmissions per minute, 37/38/39 gap
distributions, time Current, queue drops and decode failures—not by the largest
RSSI number. Keep one variable changed at a time, preserve a return to baseline,
and finish OTA startup confirmation before distributing a tuning experiment.
No RF settings were changed by adding these diagnostics.
