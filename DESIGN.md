# CYD Weather Link — architecture and requirements

Status: October 21, 2026. Local firmware v0.7.3 targets the owned PanelLan
ZX2D80CE02S-V10 / SC05_X; see [firmware scope](firmware/README.md) and
[bench validation](firmware/VALIDATION.md). Live Atlas/CC1101, display/touch,
protected setup, NWS and history restoration are observed. The owner confirms
APRS.fi, WU and Weathercloud ingestion and the new friendly local address.
SDR comparison, separate CWOP acceptance, corrected hosted OTA and multiday
soak remain open. D710 and local barometer are optional; sound is removed.
The PC RTL-SDR capture service remains stopped.

## Goal

Reuse the existing AcuRite Atlas outdoor sensor with a replacement local display, local history, Weather Underground uploads, and APRS weather reporting. Provide swipe navigation between current conditions, rolling 24-hour trends, and a local internet forecast. Local readings and available history should remain usable when the internet is down.

## Selected architecture: all-in-one CYD

The user prefers one self-contained touchscreen appliance, with no Pi, separate receiver processor, or MQTT gateway in the operating path. The Pi is available but is not part of the selected architecture. On October 20, 2026 the owner identified the connected display/controller as PanelLan `ZX2D80CE02S-V10`, matching the vendor's `SC05_X` profile. It is an owned ESP32-S3 prototype candidate; the chip probe confirms 8 MB flash and 2 MB PSRAM. Display/touch and simultaneous-peripheral validation are recorded separately in [firmware/VALIDATION.md](firmware/VALIDATION.md); the selected CC1101 wiring receives Atlas packets, while debug-header supply headroom remains unmeasured. Documented specifications and their sources are in [PARTS.md](PARTS.md#owned-displaycontroller-candidate-panellan-zx2d80ce02s-v10).

```text
Atlas outdoor sensor
    | 433 MHz broadcasts
    v
CC1101 receiver + antenna -- SPI/interrupts --> CYD ESP32
                                              |-> local touchscreen display
                                              |-> weather calculations/state
                                              |-> Weather Underground over Wi-Fi
                                              |-> direct APRS-IS / CWOP over Wi-Fi
                                              |-> Weathercloud over Wi-Fi
                                              |-> opt-in Windy/PWSWeather adapters
                                              |<- local NWS alerts/forecast over Wi-Fi
                                              `-> optional UART -> serial level interface
                                                          -> D710 GPS/weather port

Optional pressure sensor -- I2C -------------> CYD
```

One enclosure and one controller, with the CC1101, optional pressure sensor, and optional serial interface as peripheral hardware. The existing SDR remains a development reference only. There is no cloud dependency for local weather reception/display or the serial weather output.

## Hardware and feasibility gates

- Owned PanelLan `ZX2D80CE02S-V10`: start from the documented `SC05_X` configuration (ST7789 8080 parallel display and FT5x06 touch), not a generic ILI9341 CYD profile. Verify actual flash/PSRAM, board revision, schematic, and exposed GPIO before pin assignment. Resolve the six expansion GPIO budget without reserving pins for SD; verify access to I2C and, if the optional D710 output is fitted, a correctly interfaced UART.
- Add a 433 MHz CC1101 module with antenna. Use rtl_433_ESP as the initial decoder candidate, restricted to the required AcuRite decoder where practical. Confirm this Atlas works on the selected build.
- Add a BMP280/BME280 pressure sensor to replace the original console barometer if pressure is wanted.
- Optional D710 output: add a serial electrical interface matched to the exact port; a MAX3232-class interface is a candidate if RS-232 levels are required. No USB host or Pi adapter is assumed in the final appliance.
- Check GPIO availability for receiver SPI and interrupt lines, pressure I2C, and weather-output UART alongside the display, touch, backlight, boot straps, and programming port.
- Sound is excluded from this hardware version; future audio needs a separate hardware/pin review.
- SD packet logging is optional following the owner's October 20, 2026 requirement change. Normal reception, display, history, and outputs must work without a card. Allocate GPIO to required peripherals first; any later SD option needs its own verified bus/pin budget.
- If the particular CYD lacks enough usable pins or memory, select a more suitable ESP32 display board for the same all-in-one design. Do not silently reintroduce a gateway.
- Verify receiver range, antenna placement, power supply capacity, and interference from the display/Wi-Fi in the assembled enclosure.
- For the owned PanelLan, the owner selected debug pin 2 (+3.3 V), with
  debug pin 7 ground, as the direct-power CC1101 bench option. The manufacturer
  does not specify spare current; verify loaded voltage and reception with
  Wi-Fi/display running before accepting it. The 5 V expansion rail plus a
  separate 3.3 V regulator remains an alternative. Exact parts/pins are in
  [PARTS.md](PARTS.md#implemented-cc1101-pin-allocation) and
  [firmware/WIRING.md](firmware/WIRING.md#power-from-the-panellan).

## Firmware responsibilities

Use a single embedded application, initially targeting PlatformIO/Arduino with rtl_433_ESP and a suitable display library. Select the exact display stack after identifying the CYD. This replaces WeeWX and its uploaders in the final device.

1. Capture and decode Atlas broadcasts without blocking on screen rendering or internet requests.
2. Merge packet types into a timestamped observation store with validity and age per field.
3. Maintain rain deltas/totals, defined wind averaging windows, and the displayed maximum wind reading over the trailing five minutes.
4. Refresh the touchscreen from observation snapshots, with swipe navigation between Home, 24-hour Trends, and Forecast. Build timestamped history summaries without blocking capture or logging.
5. Upload fresh observations to Weather Underground using HTTPS.
6. Maintain an authenticated APRS-IS session with bounded queues, reconnect/backoff, and a configurable reporting interval.
7. Optional addition: emit a verified Peet Bros.-compatible serial stream for the D710 independently of internet connectivity. This is not a core acceptance requirement.
8. Save configuration and essential persistent weather state with controlled flash-write frequency. Raw decoded-packet logging is optional and disabled by default; no SD card is required.
9. Retrieve active local NWS alerts for configured station coordinates without blocking reception. Track alert-fetch health separately from local measurements and output connections.
10. Apply orange full-screen watch and red warning themes; this version uses visual alerts only.
11. Retrieve a local NWS day/night and hourly forecast for configured station coordinates in a bounded background task. Cache the last good forecast, expose its source/age, and track forecast health independently of alerts and local measurements.
12. Support configured CWOP participation and selectable Weathercloud, Windy.com, and PWSWeather observation uploaders, reusing the observation model while keeping destination identity, scheduling, credentials, and health separate. Focus initial PWS upload work on Weather Underground, Weathercloud, and Windy. Validate APRS/CWOP routing to avoid duplicate uploads.

Use separate tasks or nonblocking scheduling with explicit ownership of shared state. Network outages, TLS handshakes, and screen updates must not stall receiver processing or serial output. Benchmark memory with display buffers, decoder, Wi-Fi, and TLS active together. Watchdog recovery and brownout/restart behavior are acceptance requirements.

Timekeeping: use UTC internally, NTP when available, and configured local timezone rules for daily totals. After a cold start without valid time, show time validity and withhold outputs requiring trustworthy timestamps until time is established; an optional RTC can support offline cold starts.

## Why this is feasible but needs validation

The ESP32 can host these functions in principle, and a CC1101 avoids the SDR. Feasibility on the owned CYD depends on its exposed pins, memory, library compatibility, and reception under simultaneous display/network activity. This is custom firmware, not a confirmed drop-in configuration.

## Storage and optional packet logging

The owner removed SD logging as a requirement on October 20, 2026. The appliance
must operate without an SD card or raw packet archive. Existing PC reference
captures and reports remain preserved; their logger remains stopped unless
explicitly restarted.

Keep the rolling 24-hour observation summaries in bounded RAM/PSRAM. Plan compact
history checkpoints and forecast caching in a bounded internal-flash partition,
with schema/version checks, original timestamps, controlled write frequency,
and visible recovery gaps after abrupt power loss. Choose the partition layout
and measure space/wear before implementation. Essential configuration and rain
state remain persistent; do not write every received packet to flash.

A later opt-in packet logger may use SD or a development interface. If added,
retain decoder fields/units, UTC validity and receive timing, preserve repeated
copies for analysis, and keep diagnostics separate. Bound queues and expose
write/drop errors without stalling reception. Exclude raw RF/IQ, secrets,
network payloads, and configuration dumps. No SD pin allocation, purchase,
connection indicator, or logging task is required by the baseline.

## Observation handling

- Bind to the household sensor ID and channel; ignore neighbors' stations.
- Combine Atlas packet types into one observation without treating absent fields as zero.
- Track receive time and age per field because different readings arrive in different packets.
- Store explicit units internally and convert at display/upload boundaries.
- Deduplicate repeated broadcasts.
- Persist rain-counter state; handle resets, wraparound, restart, local midnight, and daylight-saving transitions.
- Calculate rain for the last hour, last 24 hours, and local day separately.
- Use documented wind averaging and gust windows for each output; do not rename a raw sample a gust.
- Preserve measured station pressure separately from derived sea-level pressure; configure elevation and validate the conversion.
- Treat missing/invalid/stale measurements as unavailable. Never upload old readings with a new timestamp.
- Keep credentials in device configuration rather than source code; redact them from logs and exports.

### Atlas rain counter and useful totals

The [rtl_433 25.12 Atlas decoder](https://github.com/merbanan/rtl_433/blob/25.12/src/devices/acurite.c)
extracts a nine-bit bucket-tip counter, scaled by 0.01 in per count. Its range
is 0.00–5.11 in; the next tip wraps to 0.00 in, a modulo period of 5.12 in
(512 tips). This is a running sensor counter, not a calendar-day total.
The rollover is documented by the decoder; a rollover or power-reset event
has not yet been observed on this station during our capture.

- Preserve the source `rain_in` field/units when optional logging is enabled; calculate using integer tips
  internally to avoid floating-point accumulation errors.
- Maintain a persistent last accepted counter and accumulated increments.
  Deduplicate repeats; the first counter is a baseline, not newly fallen rain.
- For a credible wrap, use `(new_count - old_count) modulo 512`. For example,
  5.10 → 0.02 in represents four tips, or 0.04 in. Require continuity and
  plausible rate/timing before accepting a wrap. A decrease alone cannot
  distinguish rollover from sensor reset, replacement, or bad data.
- Flag ambiguous discontinuities and re-establish a baseline without adding
  an invented wrap or subtracting from prior rainfall. Record the event and
  affected totals' incomplete status. Verify battery/power-reset behavior on
  hardware later; do not reset the outdoor station just to clear a display.
- Derive `Rain today` using local civil midnight, independently of the raw
  sensor counter. Retain separate rolling hour/24-hour totals and capture-total
  diagnostics. Archive the previous day and preserve totals across restarts.
  A day may contain 23 or 25 hours at daylight-saving transitions.
- Mark startup partway through a day as partial; pre-capture rain cannot be
  assigned to that day from the starting counter alone. During reception gaps,
  counter differences can recover accumulation only when reset/wrap ambiguity
  is excluded. Preserve the interval of uncertainty; do not silently allocate
  a gap-spanning increment to one side of midnight or a rolling-window boundary.
- Validate repeated counts, normal increase, 5.11 → 0.00, skipped-count wrap,
  unexpected decrease, receiver reboot, sensor replacement, gaps spanning a
  boundary, local midnight, and daylight-saving changes before uploading totals.

### Displayed five-minute wind gust

The requested home-screen value is `Gust 5m`: the highest valid wind-speed
reading received from the bound Atlas during the trailing 300 seconds. Use the
decoded `wind_avg_mi_h` field from all supported Atlas packet types that carry
it, retaining its source name and units in any optional packet log. The
[rtl_433 Atlas decoder](https://github.com/merbanan/rtl_433/blob/25.12/src/devices/acurite.c)
supplies that wind-speed field in each Atlas message. This display statistic
is a maximum of received station readings. AcuRite's
[wind-speed glossary](https://www.acurite.com/pages/learn-glossary-wind-speed)
describes the Atlas as transmitting every ten seconds with the highest speed
from that interval. This manufacturer description supports using the reports
for peak tracking; it is not a bench verification of the sensor's internal
sampling or a claim of a standard three-second meteorological gust.

The five-minute window also matches the APRS weather `g` field: the
[APRS protocol reference, chapter 12](https://www.aprs.org/doc/APRS101.PDF#page=74)
defines it as peak wind speed over the preceding five minutes. The protocol's
wind-speed field uses a separate sustained one-minute value. Keep their
calculations distinct when implementing uploads.

- Use a sliding window `(now - 300 seconds, now]`, rather than fixed five-minute
  blocks. Recalculate as time passes, including when no new packet arrives.
  Expire an old maximum at 300 seconds and allow the displayed value to fall.
- Deduplicate each broadcast's repeated copies for observation calculations;
  preserve copies when optional packet logging is enabled. Keep the original observation
  time so repeated copies cannot extend a peak's lifetime. Accept any received
  copy when earlier copies were missed.
- Track wind sample time independently of temperature, rain, and other fields.
  Use monotonic elapsed time for the live window and timestamped valid records
  for offline review. Preserve the selected maximum's observation time for
  the wind detail page.
- Flag incomplete history during the first five minutes after startup and
  after reception gaps; validate the gap threshold against Atlas cadence.
  Apply the wind field's freshness rule independently: show an in-window cached
  peak as stale when current wind reception is stale. With no valid samples
  left in the window, show `—`; a valid measured zero remains zero.
- Keep current wind speed and `Gust 5m` distinct on screen. Upload and serial
  outputs retain their separately documented protocol-specific wind windows.

The preserved PC reference captures contain the wind samples needed to replay
this calculation alongside lux and rain; see [CAPTURE.md](CAPTURE.md). This is
a firmware/display requirement, not an implemented embedded display. It adds
no hardware requirement and does not change the specifications in `PARTS.md`.

## Display draft

Provide three primary pages in order: **Home**, **Trends**, and **Forecast**.
Swipe horizontally between them, with visible page controls and position;
scroll lists vertically. [SCREENS.md](SCREENS.md) defines gesture handling,
shared layout, rolling 24-hour charts, history persistence, and forecast cache
behavior. Keep the alert strip and orange/red alert themes on every page.
History summaries use bounded RAM/PSRAM with compact internal-flash checkpoints;
forecast caches use bounded internal flash separately from any optional packet
log. Size buffers and benchmark memory, flash-write latency, and bus contention
on the chosen profile. No additional controller or peripheral
is required by these pages; see the hardware reconciliation in [PARTS.md](PARTS.md).

The proposed 320×240 landscape home screen is specified in [HOME_SCREEN.md](HOME_SCREEN.md), with [normal, watch, and warning previews](concepts/home-screen-v2.png) and the [original offline preview](concepts/home-screen.png). Show large outdoor temperature and trend, a wind compass/speed/gust, humidity, daily rain, pressure, and per-field freshness. The station has a lux sensor: show its reading alongside a small estimated-sky graphic. A calibrated clear-sky brightness comparison may support coarse estimated cloudiness; label the inference, validate it against observed conditions, and show unknown when uncalibrated, at night, or with insufficient inputs. Lux is not a direct cloud-fraction measurement. Secondary pages expose history, UV/lighting detail, lightning if fitted, battery status, and upload status. Source-aware [weather assets](assets/weather/README.md) provide twelve SVG/PNG variants without implying unsupported sensing capabilities.

Show sensor reception, local network connection, and external upload health separately. Display a visible stale indicator after a configurable timeout. On boot, saved readings must retain their original age and must not appear as fresh observations.

Reserve a persistent, tappable strip for the highest-priority NWS alert and alert count. Full alert text and instructions open on a detail page. When no alerts are returned, show the last successful check; on a failed or stale check, show unavailable/stale status and retain previous alerts as unverified. Never equate an internet outage with an all-clear.

An active local NWS watch makes the full screen orange; any active warning makes it red, overriding watches. Retain matching card colors and readable text. Classify product type independently of CAP severity; advisories/statements stay distinctly labeled. A failed check retains the last known alert theme with `NWS UNVERIFIED` and check age until a complete fresh snapshot resolves it. Silence and acknowledgement do not clear the alert or theme.

## Local NWS forecast

NWS is the proposed forecast source, sharing the configured location with
alerts. Resolve `https://api.weather.gov/points/{latitude},{longitude}` and
follow its `forecast` and `forecastHourly` links, as documented by the
[NWS API](https://www.weather.gov/documentation/services-web-api). Retain seven
days of day/night periods and the next 24 hours of hourly forecasts for the
Forecast page. Proposed refresh is every 30 minutes, honoring caching and
retry headers; alert checks take priority. Show issue time separately from
last successful check, preserve valid cached periods offline, and label stale
or unavailable forecasts. Location remains unconfigured until supplied.
Full UI, memory bounds, and failure behavior are in [SCREENS.md](SCREENS.md).

## Local NWS alerts

Add a configurable HTTPS client for `https://api.weather.gov/alerts/active?point={latitude},{longitude}&status=actual`, with an identifying User-Agent and GeoJSON response. Use station coordinates, not IP geolocation. Normal polling is every 30 seconds, following the owner's October 20 update, with caching, bounded parsing, timeouts, and retry/backoff. See [HOME_SCREEN.md](HOME_SCREEN.md) for update/cancellation, priority, freshness, pagination, and cache rules. All fetching remains on the device; no gateway is required.

Validate memory and reception with NWS TLS/JSON processing, all configured uploaders, history checkpointing, and the display active. Local weather reception, display, and serial output remain operational when the alert service is unreachable. Initial NWS operation is observed; extended outage/transition validation remains open.

## Sound removed from this hardware

The owner removed sound from this device; [audio plans](reference/FUTURE_AUDIO.md)
are archived for a possible larger-display version. Do not reserve audio GPIO
or add sound/mute controls to the current PanelLan acceptance scope.

## External reporting

Weather Underground: register a personal weather station and use its station ID and upload key with the embedded HTTPS uploader. Start with a proposed 60-second upload interval, subject to service settings and available fresh observations.

APRS internet: direct APRS-IS remains the requested primary ham route. Configure callsign/SSID, authenticated login, station coordinates, and weather symbol. The proposed internet reporting interval is now five minutes, aligning with NWS CWOP guidance and APRS-IS weather-beacon limits; this replaces the earlier ten-minute internet default. Implement reconnect/backoff and suppress stale reports. Keep radio beacon timing separately configurable.

CWOP / NOAA MADIS: support participation as an intended distribution target, requiring configured registration and verified ingestion. Reuse APRS weather encoding and evaluate whether the existing registered ham APRS feed provides the required collection before adding another send path. Assigned-CWOP-ID and authenticated ham/APRS-IS profiles have distinct identity, server, and authentication settings; do not treat them as interchangeable or send duplicate observations blindly. The WeeWX CWOP uploader is a reference, not the embedded implementation or proof of direct APRS-IS compatibility.

Additional PWS destinations: prioritize Weathercloud alongside Weather Underground, then Windy.com Stations using its current 2026 API. Weathercloud's documented standard cadence is ten minutes, with one-minute uploads limited to Pro/Premium accounts. Plan PWSWeather as a later adapter. Make each destination independently selectable and require station configuration before enabling it. This order uses published size evidence where available and project fit where comparable counts are absent; it is not a verified popularity ranking of ham users. Met Office WOW is removed as a new target following its announced 2026 retirement. WeatherWise.app advertises live station viewing, but its owner-upload/API path is unverified. A Meshtastic bridge depends on a demonstrated local mesh need and a separate hardware/interface study. Detailed sources, priorities, and validation gates are in [WEATHER_SHARING.md](WEATHER_SHARING.md).

Each configured exporter has bounded scheduling/retry work and its own health state. Respect destination-specific field semantics, wind/rain windows, units, and timestamp handling. Keep reception and history independent of every uploader and any optional packet logger. The internet destinations reuse existing Wi-Fi and add no component requirement to [PARTS.md](PARTS.md); no mesh radio or additional controller is selected.

Optional radio output: emulate a weather station into the D710 GPS/weather port, allowing the radio's built-in APRS function to create and transmit weather beacons. No additional TNC is needed for this path. Direct APRS-IS and serial output are independently switchable. Decide station identity and beacon coordination before operating both, since RF reports may also reach APRS-IS through an IGate.

## Optional Kenwood D710 serial emulation

Kenwood documents Davis and Peet Bros. weather inputs. Select Peet Bros. ULTIMETER emulation as the first implementation target: its published ASCII format is simpler to generate than a Davis logger protocol. Kenwood specifies 2400 baud for the ULTIMETER 2100 on the TM-D710A/E, using Menu 602 WEATHER (Peet Bros.) with output OFF.

Peet Bros. documents 2400 baud, 8 data bits, no parity, one stop bit. Candidate record formats are data logging (`!!`) and packet mode (`$ULTW`). The Kenwood documentation reviewed establishes station compatibility, but does not establish which record variants its parser accepts. Bench-test both; select and document the verified format before calling the emulator compatible.

The emulator must perform the documented fixed-width hexadecimal encoding, field scaling, signed-temperature handling, rain-counter conversion, and CR/LF termination. Missing fields must use a supported unavailable representation, or the record must be withheld; do not fabricate zero pressure or humidity. Verify the radio's behavior after serial input stops, since it may retain previously received weather.

Use a proper serial electrical interface. The ESP32 UART is 3.3 V logic; do not connect it directly to the radio based only on a matching baud rate. Verify the exact D710 model's GPS-port pinout, polarity, and signal levels. Use an appropriate UART transceiver such as MAX3232 if the verified port requires RS-232 levels. This is the GPS/weather input, not the packet-data connector or PC control port.

First validate encoded records on a serial capture, then confirm all six weather measurements on the D710, then inspect an actual transmitted weather packet. Allow the acquisition delay Kenwood documents (5–10 minutes).

Local operation during an internet outage requires local Wi-Fi/LAN and gateway power. RF reporting during an outage additionally requires the radio path and backup power if desired.

## Build sequence and acceptance criteria

1. Identify CYD and, if optional serial output is selected, D710 variants. Obtain the CYD schematic and map all required peripherals before wiring.
2. Compile a minimal CYD + CC1101 receiver build. Compare its Atlas output with the existing SDR over at least a day.
3. Add the display and pressure sensor; verify packet reception remains reliable under screen activity.
4. Implement persistent rain state, averaging, timestamps, and stale-data handling. Validate resets and counter wraparound with meaningful packet fixtures. Replay wind packets to verify the rolling five-minute maximum, expiry exactly at 300 seconds, a lower value after the peak expires, repeated copies, a missing first copy, invalid/missing wind, zero wind, startup coverage, reception loss, and service/device restarts.
5. Add direct APRS-IS with validated CWOP participation and selectable PWS uploads: Weather Underground and Weathercloud first, Windy third, and PWSWeather later. Test encoding, registration/actual ingestion, destination-specific cadence and failures, retry/backoff, duplicate routing, stale fields, memory use, and continued reception with all configured uploaders active.
6. Optional D710 path: implement Peet Bros. UART emulation. Capture and validate records, then check measurements and weather packets on the D710.
7. Test sensor loss, Wi-Fi loss, internet loss, restarts, invalid time, and stale serial-input behavior at the radio.
8. Package as one appliance and run a multiday soak test with all outputs enabled.
9. Validate NWS empty/active/multiple/updated/cancelled alerts, stale cache, invalid time, location changes, payload limits, and network failures while reception and serial output continue. Calibrate and assess lux-based sky estimates independently before enabling them by default.
10. Validate whole-screen orange/red alert priority and stale-state labels across normal palettes.
11. Validate swipe navigation and all-page alerts, rolling 24-hour history with gaps and restart recovery, and forecast caching/expiry/failures using [SCREENS.md](SCREENS.md#acceptance-checks). Confirm these tasks do not interrupt reception, history updates, uploads, or serial output.

## Remaining design decisions

Board revision, 8 MB flash/2 MB PSRAM, Atlas message family, 433.92 MHz profile
and seven-day archive size are established. Current owner settings supply
location/timezone and destination credentials; keep those private.

- Measure loaded debug-supply voltage/headroom under Wi-Fi/display/RF load.
  Six expansion GPIOs are used by CC1101; optional I2C/UART needs verified allocation.
- Decide whether offline cold starts need an RTC; history restoration needs valid UTC.
- Verify separate CWOP routing/collection and field accuracy at each destination.
  Ground elevation differs from the Atlas's 30 ft mounting height.
- Verify exact D710 variant/port levels/cable only if that output is pursued.
- Broaden station support with reviewed captures, field/identity/integrity rules
  and appropriate OOK/FSK receiver profiles; decoder availability is not compatibility.

## Technical references

- [rtl_433 AcuRite decoder](https://github.com/merbanan/rtl_433/blob/master/src/devices/acurite.c)
- [weewx-sdr](https://github.com/matthewwall/weewx-sdr)
- [WeeWX upload configuration](https://weewx.com/docs/latest/usersguide/uploading/)
- [rtl_433_ESP](https://github.com/NorthernMan54/rtl_433_ESP)
- [Kenwood TM-D710A/E in-depth manual, section 6.2](https://www.kenwood.com/i/products/info/amateur/pdf/TM-D710AE_IDM_R1.pdf)
- [Peet Bros. serial formats](https://www.peetbros.com/shop/custom.aspx?recid=29)
- [Weather Underground upload protocol](https://support.weather.com/s/article/PWS-Upload-Protocol)
- [APRS-IS connections](https://www.aprs-is.net/Connecting.aspx)
- [APRS protocol specification](https://www.aprs.org/doc/APRS101.PDF)
- [CWOP guide hosted by NWS](https://www.weather.gov/media/epz/mesonet/CWOP-OfficialGuide.pdf)
- [AcuRite Atlas display with built-in barometer](https://www.acurite.com/products/acurite-atlas-weather-station-with-gray-hd-display-and-solar-power-pack)
- [NWS API documentation](https://www.weather.gov/documentation/services-web-api)
- [NWS alerts service and polling guidance](https://www.weather.gov/documentation/services-web-alerts)
- [NWS alert geolocation guide](https://www.weather.gov/media/documentation/docs/NWS_Geolocation.pdf)
- [NWS watch/warning/advisory definitions](https://www.weather.gov/sjt/WatchWarningAdvisoryExplained)
- [MAX98357A speaker amplifier](https://www.analog.com/en/products/max98357a.html)

## Setup location picker — owner request, October 20

Implemented in the protected portal; browser/endpoint checks passed and owner
the owner reports setup/display changes passed their checks. See [map assets and provider notes](assets/web/README.md).

- Preserve manual decimal latitude/longitude entry.
- Offer US ZIP-code search, followed by an interactive map in the phone/PC
  browser. Allow pan/zoom and a draggable/selectable station pin.
- The ZIP result is an initial search area, never the exact station location.
  Show the chosen coordinates and require confirmation before saving them.
- Render maps in the browser; the ESP32 serves setup and saves the confirmed
  position. Do not load a tile renderer or map database onto the panel.
- Research a geocoding/tile provider, terms, attribution, rate limits, key
  protection and privacy before implementation. Do not embed private service
  keys in browser JavaScript or promise a free provider without validation.
- Map/search needs browser internet access. Manual entry must continue to work
  on the device's local setup network when internet is unavailable.
- Distinguish station location from a weather-service station identity. A map
  selection must not silently enable external observation uploads or change
  an APRS identity. Keep NWS/sun lookup enabled/disabled state explicit.
- Use a dropped/dragged point for ground-elevation lookup, not the ZIP center.
  Show source/units and retain manual entry. Missing results must not become
  sea level or retain a different point's elevation. Ground is separate from
  the Atlas's approximately 30 ft mounting height.

## Header, transmitter health and setup security — latest owner updates

### Post-setup settings access — implemented v0.6.0

Owner request, October 21: after initial setup, opening setup from the display
should show a **More settings** screen rather than the initial Wi-Fi joining
instructions. Suggested text: “For more settings, open http://<device IP>.”
Use the current LAN address and show **Username: admin** with password guidance.
Show the generated initial admin password only while it remains the active
default; after a custom password is set, show **Password: your custom password**
and never reveal that password. Preserve the separate setup Wi-Fi/admin keys.

Use saved home Wi-Fi plus a confirmed sensor binding as the completion signal;
location and optional sharing services are not prerequisites. Keep first-run
provisioning and a deliberate Wi-Fi recovery path available when needed. This
is backlog work; the current firmware retains its existing setup screen.

Show callsign/SSID in the center slot. RF/WiFi/APRS are right-aligned; sound is omitted. APRS/CWOP gets a separate session indicator requiring acknowledged
login, with verified authentication for the ham route. Keep the session alive
independently of radio freshness; withhold weather while data is stale.

The owner reports occasional Atlas repeated-single-packet or silent-transmitter
faults until power cycling. Show Data stale when packets arrive without a
required type/field update for ten minutes, or no accepted weather-value change
for ten minutes. Show Disconnected after five minutes without bound packets.
Keep receiver initialization distinct. The unchanged-data warning is a heuristic
and cannot alone establish a frozen station. Display field ages use ten minutes;
upload and archive completeness retain their separate two-minute freshness gates.
See HOME_SCREEN.md for exact state precedence. No component change is required
in PARTS.md.

A generated setup Wi-Fi key may be displayed for local provisioning; a
user-chosen admin password must never appear on the display, in API responses,
serial records or exports. Split those credentials, preserve admin login on
legacy migration, and never permit an API request to reset the private/display
classification. The custom password and visible Wi-Fi key must differ.

## Optional GitHub firmware updates

Provide protected browser Check/Install controls plus separately opt-in daily
checks and automatic installation. Start disabled with no repository selected.
Validate version, board, flash/partition layout, image size/header, HTTPS and
SHA-256. Write the inactive application slot; preserve NVS and weather state.
Keep OTA work off the NWS worker, defer unattended installation during active
or unverified NWS hazards, and confirm a new image only after startup health
checks. Validate interrupted download and rollback on hardware before enabling
unattended use. [UPDATES.md](firmware/UPDATES.md) defines release preparation
and current validation limits; this does not publish anything to GitHub.

## Rolling rain and forecast graphics — October 20 owner updates

Latest owner revision: Home shows **RAIN TODAY**, resetting at configured
local civil midnight, rather than the earlier requested rolling 24-hour tile.
An existing baseline permits a partial zero display immediately on the new
date, before the next rain packet. Keep the previous packet's date/counter
for continuity and midnight-boundary checks. Startup, stale reception, and
ambiguous rain across midnight remain incomplete; no valid date/baseline
means unavailable. Use this same daily total for WU/Weathercloud and APRS `P`.
Keep separate rolling hour/24-hour totals in rain details and service exports.
Persist the bounded counter-interval ring with CRC-protected history. Full
hour/day rolling coverage gates interval exports; missing intervals or
ambiguous boundaries cannot become complete totals.

Use fresh, current NWS hourly conditions for the main icon, labeled as a
forecast. Hide it offline or when NWS is stale/unavailable. This replaces
lux-based sky graphics as the initial implementation; lux remains a measured
local field, with optional calibration research separate.

The [Sunrise-Sunset.org v2 API](https://sunrise-sunset.org/api) supplies daily
sunrise/sunset plus moon phase/illumination in one HTTPS request, cached by
local date/location. Show its required attribution. Sunrise/sunset select
sun versus moon; use NWS day/night metadata if solar times are unavailable.
These are predicted astronomical/forecast values, not local sky observations.


## Deferred multi-station discovery and community validation

Implementation update: v0.5.1 provides user-confirmed Atlas discovery on the
existing 433.92 MHz OOK profile, with Iris/Notos detection previews. It retains
the existing station until settings are saved. Broader adapters, FSK profiles
and community hardware validation remain follow-up work; the following gates
continue to apply to those additions.

Owner direction: first verify the existing Atlas over the physical CC1101.
Broader weather-station support is a follow-up milestone, not a claim about
current firmware. Preserve manual configuration alongside discovery.

The October 21 [station-support research](STATION_SUPPORT.md) prioritizes Iris,
selected Sainlogic and verified 433 MHz Fine Offset variants, with separate
frequency/modulation and integrity gates. These are engineering priorities,
not a measured popularity ranking or a change to the hardware baseline.

Proposed setup flow:

The owner's clarified flow is to listen during setup, count several independent
valid transmissions from the same device, then ask whether it is their station.
The [decoder/discovery contract](DECODER_SUPPORT.md#confirmed-owner-discovery-during-setup)
proposes three independent sightings as an initial confidence threshold. The
owner approved a setup prompt explaining automatic detection can take up to
five minutes, with automatic/manual choices, visible progress, early candidate
suggestions and scan-again/manual choices at timeout. Keep the existing binding until confirmation;
discovery data must not enter operational archives or external uploads. Limit
initial candidates to reviewed 433 MHz profiles. These are specified behaviors,
not a claim that discovery has been implemented or RF-validated.

Automatic discovery is the default user path. Manual configuration belongs
under Advanced; normal setup must not require the user to know or enter a sensor
ID. Lead confirmation with the detected family and live readings, with technical
identity/confidence information in expandable details. Save the decoded identity
automatically only after the user confirms the candidate.

- Listen on the selected supported RF frequency/modulation for a bounded scan
  window. The scan duration and required observation count remain to be tuned
  against real station transmission intervals; do not hardcode a universal cadence.
- Suggest candidates only after several valid observations across separate
  transmissions. Repeated copies in a single burst must not inflate confidence.
- Show decoded model/protocol, sensor ID, channel when present, last reception,
  observation count and available sample readings. A decoder's model label may
  describe a product family rather than the exact retail model.
- Require the user to confirm the intended candidate. Never automatically bind
  to the first/strongest device or replace a missing station with a neighbor.
- Provide manual model/protocol, frequency, ID and channel setup where applicable.
- Persist the selected identity using all protocol-relevant identifiers. Keep
  unknown/missing fields unavailable and prevent different stations' packets
  from silently becoming one observation. Combining separate sensors into one
  station is an open design question, not an accepted implementation requirement.

Before enabling another family, define field/unit mappings, expected cadence,
freshness requirements, available measurements, gust semantics and rain-counter
resolution/reset/wrap behavior. Atlas-specific 37/38/39 completeness and its
rain-counter assumptions must not become universal rules. Publish separately
whether a model is decoder-supported, fixture-tested or verified on hardware.
The current 433 MHz module does not establish compatibility with every band or
modulation supported by desktop rtl_433; check each proposed radio profile.

Community validation TODO:

- Recruit volunteers after Atlas reception and the initial build procedure are
  verified; provide the exact supported board/radio profile and wiring guide.
- Prepare an issue/report template with station make/model/region/frequency,
  firmware version, radio/antenna, observed fields and timestamped decoded JSON
  spanning complete transmission cycles. Ask for rain/wind examples and missing,
  stale or reset behavior when practical; note uncertainty rather than inventing it.
- Provide a capture/submission procedure suitable for additional station families;
  the existing Atlas-bound PC tool is not a generic compatibility collector.
- Exclude upload keys, Wi-Fi/admin credentials, full-flash backups and unnecessary
  personal/site details from submissions. Review captures before making fixtures public.
- Turn reviewed samples into decoder/normalization regression fixtures, then
  ask volunteers to compare the display with their reference receiver/console.
- Keep discovery and broader decoder work deferred until the owner's Atlas
  validates reception, supply stability and simultaneous network/display load.


## Seven-day on-device summaries — October 21 owner update

Retain the rolling 24-hour minute history for display and add a separate bounded
seven-day archive of five-minute summaries. Preserve temperature mean/min/max,
humidity mean, light mean/peak, received wind mean/peak, observed rain increments
and reception/coverage counters. Missing readings, restart gaps and uncertain
rain must remain identifiable. Only live bound-station data with valid UTC may
enter the archive; replay must not write it or change uploader eligibility.
Use fixed 64-byte CRC records (2,016 slots, 126 KiB payload), protect a 512 KiB
filesystem reserve and preserve both OTA slots and existing checkpoint schema.
Provide authenticated browser CSV and USB download for later analysis. Keep
archive writes bounded and reception responsive; document flash-block overhead
and partial current-interval loss on power interruption. See
[RECORDING.md](firmware/RECORDING.md) for format, recovery and export behavior.


### Display-only pressure fallback (October 21)

Until a local barometer is fitted, the BARO tile may show a fresh observation
from a nearby NWS reporting station, explicitly labeled NWS with source/type/age
in details. Prefer sea-level pressure and distinguish the reported barometric
fallback. Apply a 90-minute observation-age limit; missing, invalid, future,
offline or expired values display unavailable. This external observation is not
Atlas data and must not enter local archives or observation uploads.

## Local browser dashboard

The owner requested phone visibility using the protected setup page. Version
0.5.0 adds main-screen readings, gapped 24-hour trends, full cached local NWS
forecast/alert details and setup-first onboarding. Read-only paging reuses
existing history and provider caches; display presence is separate from strict
upload validity. No additional controller, storage or component requirement is
introduced, so PARTS.md needs no hardware change. The dashboard contract and
validation are in [firmware/WEB_DASHBOARD.md](firmware/WEB_DASHBOARD.md).

Post-setup completion uses saved home Wi-Fi plus a confirmed station binding.
The implemented More settings view shows URL/admin guidance, hides a custom
password and retains a deliberate recovery/details button.

## Display preferences and shared themes — current implementation

Local v0.7.2 includes the later owner changes to the v2 reference: WX LINK,
larger centered callsign, configurable date prefix and 12/24-hour clock,
right-aligned RF/WiFi/APRS statuses, no sound icon and seconds since the last
decoded bound-station packet. Repeats prove reception independently of field
freshness. Receiving replaces the partial-cycle Collecting label.

NWS attribution sits above the weather icon. Local klux includes UV index and
full level (including Moderate); stale/missing UV is unavailable. BARO may use
fresh nearby NWS pressure, with source/type/age in details; it is display-only,
excluded from Atlas uploads/archive. Detailed trends preserve data gaps; only
the small Home sparkline bridges observed points across missed minutes.

Trends and Forecast return Home after 60 seconds without touch; setup, alerts
and measurement details are excluded. Keep-on/Dim/Off backlight preferences do
not sleep the MCU or pause RF, history, NWS or uploads. First-touch wake consumes
the gesture before normal controls resume. Touch/hold/release reset inactivity.

Station & Wi-Fi → Display & clock offers Midnight (original), Matrix, Amber,
Ice Blue, Violet, Paper and Night (low light). Browser changes preview locally;
Save and restart persists one palette for screen and webpage. Old settings
retain Midnight; invalid/nonstring themes are rejected. Shared JSON generates
RGB565 colors, embedded CSS and recolored original SVG weather artwork without
changing geometry. Watch/warning themes retain distinct colors; Paper uses a
darker readable stale orange. Sound is excluded from this hardware version;
[earlier audio plans](reference/FUTURE_AUDIO.md) are archived for larger displays.

Night uses near-black surfaces, subdued amber text/artwork/moon, and softer
olive NWS all-clear/connected indicators. Backlight is 24/255 normally, 2/255
when idle-dimmed and zero for Off; other themes remain 160/16. Wake returns to
the selected level. After display initialization, boot leaves the backlight
off until settings and the first themed frame are ready. Actual dark-room
appearance/wake checks remain owner validation.

Home uses current provider moon phase. Forecast icons estimate the phase at
each period midpoint from provider illumination/direction, anchored at noon of
the current local reference date and advanced with a mean synodic cycle.
This is a small-icon estimate, not an ephemeris. Invalid/missing current-day
lunar data remains unknown; hemisphere orientation, crater texture and cloud/
fog/precipitation overlays are preserved. No extra provider requests are made.

## Friendly settings address — v0.7.2

Advertise wxlink.local with mDNS and HTTP on TCP port 80. First-run and More
settings views show http://wxlink.local prominently, with current numeric AP/
LAN IP fallback. The browser header links both. Existing Basic authentication,
CSRF and private custom admin credentials are unchanged; no secrets are advertised.
Failed mDNS startup retries every 30 seconds and does not block core weather
operation. Existing setup DNS and 192.168.4.1 remain available. Default name
assumes one unit per local network; clients without local multicast support
can use the IP. No component, GPIO or storage-layout change is required.


## v0.8.0 owner-test candidate

Firmware adds independent PWSWeather and Windy.com v2 credentials, scheduling and
status, with live ingestion pending owner verification. All exporters still use
strict live field freshness and protocol-defined units; a metric/imperial display
setting never alters archival or uploader units. Hostnames are saved per device
and change mDNS/DHCP/browser/display addresses; numeric IP is always available.

Factory reset requires authenticated typed confirmation or a deliberate physical
hold/release/hold sequence for forgotten credentials. It clears only wxlink
settings and enumerated weather history/cache files, retains firmware/partitions,
and completes a durable reset marker last so interrupted work retries at boot
before workers start. Logical deletion is not forensic credential erasure.

NWS AWC pressure fallback and an opt-in calibrated daylight lux estimator add no
hardware or GPIO. The estimator affects current sky art only and is source-labeled,
with low-sun/night/stale/uncalibrated fallback to NWS. Neither it nor NWS pressure
is passed off as local observation telemetry. Details and evidence are in
firmware/LUX_CLOUDS.md and firmware/NWS_PRESSURE.md. PARTS.md is reconciled below
with these software-only additions; the optional local barometer still requires
its own electrical and calibration review.
