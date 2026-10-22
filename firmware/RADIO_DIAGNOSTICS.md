# Atlas reception diagnostics

## Expected timing and the saved overnight baseline

[AcuRite's Atlas specifications](https://www.acurite.com/products/acurite-atlas-weather-sensor)
state transmissions every 10 seconds; wind reports every 10 seconds and the
other weather fields every 30 seconds. The owner-requested display policy warns after ten minutes without any
weather-value change and marks the station Disconnected after five minutes
without bound packets. This unchanged-data heuristic does not prove a freeze.
Individual display fields expire after ten minutes without their own update. The firmware accepts a new matching
payload after the three-second same-burst duplicate window, even if its values
have not changed. A complete packet cycle and fields younger than two minutes are still required
for uploads; the relaxed display policy does not relax service freshness.

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

## Read-only USB monitoring — available since v0.4.3

```bash
bash tools/firmware-console.sh checkpoint-save
bash tools/monitor-rf.sh captures/rf-baseline.jsonl --seconds 600
```

Included in the current firmware. Do not run another serial command while the
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


## Temporary capture-margin experiment, v0.4.4 and later

The owner authorized opening the software gate after an accepted burst was
only 1 dB above it. This command explicitly changes the margin in RAM:

```bash
bash tools/monitor-rf.sh captures/rf-margin6.jsonl --seconds 360 --margin-db 6
```

Allowed range is integer 3–15 dB; default at boot remains 9 dB. No NVS or radio
register/center-frequency/bandwidth/AGC change is made. The supported library
setter changes the margin; the effective threshold updates on its next average
recalculation. `capture_margin_db` is the requested margin; compare actual
`threshold_dbm` with `average_rssi_dbm` to confirm it has taken effect. The
host retries a lost acknowledgement idempotently. A monitor started without
--margin-db leaves the current setting alone. Stopping monitoring does not undo
an explicit margin experiment; a device restart restores the 9 dB default.

Observed October 21: on coax, decoded bursts were roughly -68 to -82 dBm with
background near -92/-93 dBm. Reception still had a later two-minute dropout.
With the small antenna and the 6 dB margin, background rose toward -63 dBm and
the adaptive threshold toward -57 dBm. After the owner confirmed the small
antenna, only one distinct temperature report (three copies) was observed over
about 201 seconds; its capture lasted 37.2 seconds, consistent with a prolonged
noise/signal capture. It is not a clean per-packet RF-power measurement.
No capture/decoder/application queue drops were recorded. The owner returned to
coax for the night and plans a VNA check. The antenna change times were not
measured precisely, so this is an operational comparison rather than a calibrated
sensitivity or packet-error-rate test. Do not lower the gate further solely on
this evidence; assess antenna matching, placement/interference and filter tuning.

## October 21 antenna-placement follow-up

A read-only v0.4.5 capture retained the default 9 dB software margin. The owner
reported poor reception with a VNA-checked Diamond SRH805S and then returned to
approximately one foot of coax with an alligator clip on its center conductor.
No firmware tuning changed during this capture.

Before recovery, 44 status samples had median reported RSSI/background of
-68 dBm and a -59 dBm capture threshold. After recovery, 63 samples had median
background -97 dBm and threshold -88 dBm. All three Atlas packet types resumed;
the capture contained 19 decoded copies, eight accepted reports, and zero
capture, decoder or application queue drops. These are receiver estimates,
not calibrated noise measurements, and the physical change time was not logged
precisely.

The result supports an antenna/placement/counterpoise or local-interference
problem; it does not establish that the Diamond antenna is defective, nor
identify the interference source. The manufacturer's SRH805S specification
includes 430 MHz and an SMA-P connector:
https://www.diamond-ant.co.jp/english/amateur/antenna/ante_3hand/ante_hand2.html

The next controlled comparison is the same Diamond antenna directly attached
versus on a short, shielded SMA extension, with fixed receiver settings and
unchanged board/power position. Keep antenna orientation and location documented
and wait for the adaptive RSSI average to settle before comparing each run.
Then vary display/USB proximity separately. A VNA match measurement alone does
not measure received noise, radiation efficiency or installed counterpoise.
Keep the working coax setup as the reference; do not lower the gate further
solely to compensate for the roughly 29 dB background-estimate difference.

A subsequent three-minute coax-only run remained Current in all 174 status
samples, receiving 18 newly accepted reports and 42 logged packet copies
covering types 37, 38 and 39. Median reported background/threshold was
-93/-84 dBm, with no receiver/application queue drops. The USB collector also
encountered four unparsable lines, so logged packet-copy counts should not be
used as a complete RF-transmission count. Native receiver and accepted-report
counters provide the independent check.

The owner confirmed the intended status policy on October 21: warn after ten
minutes without any weather-value change, and show Disconnected after five
minutes without a bound-station packet. Stable repeated weather is accepted as
new data outside the three-second burst duplicate window; the unchanged-value
warning is a requested heuristic, not proof of a frozen station. Earlier
firmware's two-minute age-based stale events cannot be attributed to steady
weather values alone. The ten/five-minute policy is included in v0.4.7; it was
absent from the first v0.4.5 readout and from the isolated v0.4.6 history fix.
