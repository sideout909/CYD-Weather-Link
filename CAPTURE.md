# Continuous Atlas reference capture

This PC-side development tool records the existing RTL-SDR reference receiver.
It does not change the single-controller appliance architecture in `DESIGN.md`
or the component choices in `PARTS.md`.

## Hardware and observation context

- CachyOS; packaged `rtl-sdr` and `rtl_433`; Python 3 standard library and
  the system's `stdbuf` utility (GNU coreutils) for immediate line delivery.
- RTL-SDR V3, serial `00000001`, receiving at 433.92 MHz and 250 ksample/s,
  automatic gain, decoder 40. All packets decoded by this decoder are logged.
- Target AcuRite Atlas: ID `679`, channel `A`.
- The entire all-in-one outdoor station is mounted on a pole 30 ft (9.144 m)
  above ground, in full sun, as reported by the owner.
- Owner reports that this Atlas can repeat one packet or stop transmitting after
  several days until its power is cycled. This is an installation observation,
  not a confirmed fault in every Atlas. Firmware tracks stale packet cycles
  separately from missing transmissions; stable weather values alone are not
  treated as proof of a freeze.
- Receiver antenna at setup: magnifying glass alligator-clipped to coax.
- The owner expects a cloudy fall day with some rain following setup. This is
  anticipated weather, not a verified observation or clear-sky calibration.
- Local timezone: `America/Detroit`; receive timestamps and file dates use UTC.
- [AcuRite's Atlas overview](https://www.acurite.com/pages/atlas-landing-page)
  confirms that the outdoor unit combines temperature, humidity, wind, rain,
  UV, and light sensing.

## Running and stopping

Current status, October 19, 2026: logging was stopped at the owner's request
and the installed user service was disabled and removed. Captured data and
reports remain in `captures/atlas/`. The source service template and capture
tool remain available for reinstalling later using the commands below.

When installed and enabled, the user service runs indefinitely and retries receiver failures
every five seconds. User lingering is enabled so the service survives logout
and starts at boot. It also survives closing this chat and terminal windows.
The PC must remain powered on; its owner reports that it stays awake. USB
permissions currently come from the active desktop session's `uaccess` rule:
an already-open receiver continues after logout, but reopening it after a USB
reconnect or reboot may need a desktop login to grant device access again.

```bash
systemctl --user status atlas-capture.service
systemctl --user stop atlas-capture.service
systemctl --user start atlas-capture.service
```

To stop collection and prevent startup at future logins:

```bash
systemctl --user disable --now atlas-capture.service
```

The service definition is `~/.config/systemd/user/atlas-capture.service`.
To reinstall it from this checkout at its current location:

```bash
mkdir -p ~/.config/systemd/user
cp tools/atlas-capture.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now atlas-capture.service
```

To run manually, first stop the service to release the receiver:

```bash
python tools/atlas_capture.py --output captures/atlas
```

## Log format and storage

Files are kept in `captures/atlas/` and excluded from source control.

- `packets-YYYY-MM-DD.jsonl`: one JSON object per decoded packet; all repetitions
  and decoder field names/units are retained. Missing fields remain absent.
- `_capture` on each packet: session identifier, session receive sequence,
  UTC receive timestamp with explicit offset, monotonic nanoseconds, and NTP
  synchronization status (`null` when it cannot be checked). Monotonic times
  are meaningful within a system boot; sessions identify service restarts.
- `receiver.log` and up to three rotated backups: receiver diagnostics and
  restart notices, separate from packet records; 2 MiB maximum per log file.
- `status.json`: atomically replaced every 15 seconds while the receiver runs;
  includes session counts, latest target lux and its receive timestamp, last
  packet time, receiver state, and free disk space. In retry state consult
  `receiver.log`; a running service alone does not prove reception.
- `session-*.json`: command, package versions, and a snapshot of `context.json`.
- `context.json`: owner-reported siting, improvised antenna, and weather notes.

Packet files append across restarts and rotate at UTC midnight. Writes flush
to the operating system per line and synchronize to disk every 15 seconds.
Abrupt power loss can lose recent writes or leave an incomplete JSON line.
On reopening a file, a partial final line is separated from subsequent records;
review tools should skip and count malformed lines. A directory lock prevents
two copies of this logger from writing the same archive simultaneously.

No packet history is automatically deleted. Collection stops with error code
78 if free space falls below 256 MiB or a write fails; inspect logs, free space,
and start it again. The service does not restart on that error. No raw RF/IQ
data or network configuration is captured.

## Reviewing lux, rain, and wind

Use type-39 packets for actual lux/UV samples; type 37 supplies temperature and
humidity, and type 38 supplies rain and wind direction. Keep receive time and age
for each field when joining packets. The three sequence-number copies usually
represent one transmission; deduplicate for charts and calculations while
preserving all originals here. A repeated rain-counter value is not new rain,
and its absolute value must not be labeled a daily total. Counter resets and
rollover require explicit handling before calculating rainfall.

The [Atlas decoder in rtl_433 25.12](https://github.com/merbanan/rtl_433/blob/25.12/src/devices/acurite.c)
documents a nine-bit rain counter: 0.00–5.11 in in 0.01 in steps, wrapping
from 5.11 to 0.00 on the next tip (5.12 in modulo period). A local-day display
must calculate its own increments and midnight boundary rather than wait for
the RF counter to clear. See [rain handling](DESIGN.md#atlas-rain-counter-and-useful-totals).
No reset or rollover has been observed in our first 10.85 hours of captured
data. The October 16 noon review's 0.46 in is the increase from 0.30 to 0.76,
not the raw counter or a confirmed full-day total. Capture began at 01:33 EDT;
rain before that is unknown. Sensor power-reset behavior remains unverified.

Full-sun siting at 30 ft is recorded for interpretation. Cloudy-day lux captures
are reference observations, not a calibrated cloud-cover classifier; later
comparison should include verified clear conditions at similar solar elevation.

Owner reference added October 16, 2026: approximately **15,000 lux corresponds
to direct sunlight around midday in fall at this installation**. The owner
expects higher readings in summer; that seasonal expectation is not yet
measured. This is a site/sensor-specific observation, not a universal sunlight
cutoff, clear-sky maximum, or numerical cloud-cover calibration. The noon
review separately measured a 16,820-lux peak at 11:52 EDT; the observation is
approximate and does not establish exact sunlight conditions at that sample.
Keep both observations and subsequent seasonal measurements in
`captures/atlas/observations-notes.jsonl` alongside the unchanged raw packet log.

The owner also reported strong gusts during the October 16 setup. Wind speed
is already retained in `wind_avg_mi_h` across Atlas types 37, 38, and 39, so this
recording can be replayed to calculate the requested `Gust 5m`: the highest
valid bound-station reading during the trailing 300 seconds. Deduplicate
transmission copies for that calculation while preserving their original
observation time. Missing readings must remain gaps. Keep derived review
results separate from the packet archive. See the
[display requirement](DESIGN.md#displayed-five-minute-wind-gust) for expiry,
freshness, and incomplete-history rules.

### Wind definitions checked during setup

- AcuRite's [wind-speed glossary](https://www.acurite.com/pages/learn-glossary-wind-speed)
  describes Atlas current reports as the highest speed during each ten-second
  transmission interval. It describes a two-minute average and a rolling
  one-hour peak separately. The decoder key `wind_avg_mi_h` is retained exactly;
  its spelling alone does not establish a two-minute averaging period.
- The [Atlas 06061 display guide](https://support.acurite.com/hc/en-us/article_attachments/4411721456283)
  labels its wind display with the latest reading, a two-minute average, and
  the peak for the current day. That model's daily peak and the generic glossary's
  one-hour peak describe different display history; identify the owner's exact
  console variant before claiming its peak window.
- The [APRS protocol reference, chapter 12](https://www.aprs.org/doc/APRS101.PDF#page=74)
  specifies the weather `g` field as the highest wind speed in the previous
  five minutes, with sustained one-minute wind reported separately. This is
  additional justification for the requested `Gust 5m` display window.

These are manufacturer/protocol definitions, not independent verification of
the Atlas's internal averaging or sampling. The raw log supports later
comparison of multiple windows without altering original readings.

## Validation

```bash
python -m unittest discover -s tools -p 'test_*.py' -v
```

Checks cover repeated packets, sparse fields, zero lux, midnight rotation,
restart appends, incomplete lines, station identity, diagnostic separation,
silent-receiver stop, and low-disk handling. Live validation should confirm all
three Atlas types and stop/start the service to verify that data is appended.

### Setup results: October 16, 2026

- Seven automated checks passed on Python 3.14.7 with `rtl_433` 25.12 and
  `rtl-sdr` 1:2.0.3-1.1.
- Live capture began at 01:33 EDT, with types 37, 38, and 39 from Atlas 679/A.
- A service stop/start preserved the first 12 packet records and opened a new
  session in the same daily file.
- Terminating only the receiver process exercised automatic recovery: the
  supervisor restarted it after five seconds, and all three types resumed.
- Latest lux during setup was zero at night. Daytime behavior and the expected
  cloudy/rainy conditions remain to be observed.
