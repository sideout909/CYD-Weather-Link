# Core firmware validation

October 21, 2026 · version 0.4.0 · `panellan_sc05_x` · owned
`ZX2D80CE02S-V10`, USB serial `68:B6:B3:34:82:48`.

## Hardware and preservation

- Device-specific USB permission rule permits read/write without root.
- esptool 4.9.0: ESP32-S3 QFN56 rev 0.1, 40 MHz crystal, 8 MB flash,
  2 MB embedded quad PSRAM. Runtime reports roughly 2.095 MB usable PSRAM.
- Original 8,388,608-byte flash and SHA-256 manifest remain private under
  `captures/hardware-backups/panellan-20261020T165529Z/`.
- Secure boot/encryption were off; no eFuse writes were performed.
- Owner confirmed display, touch and all three initial pages work, and later
  confirmed the main screen returned after the combined build's startup fix.

## Build and host checks

`bash tools/pio.sh run -d firmware` passed with the pinned dependencies.
The earlier rain/detail build used 1,433,489 application bytes. The uploaded
v0.3.0 build uses 1,516,861 / 3,342,336 application bytes (45.4%); static
RAM is 68,212 / 327,680 bytes (20.8%). Two application slots and LittleFS use
`default_8MB.csv`. Opt-in OTA is implemented; hosted installation/rollback
has not yet been bench validated.

`bash tools/test-firmware.sh` passed:

- Weather model: separate sparse-field ages, zeros, duplicate timing, exact
  five-minute gust expiry, coverage gaps, station binding, invalid/NaN/flagged
  samples, backwards time, rain baseline/reset/wrap and history overwrite.
- Reporting: APRS coordinate rounding, missing fields, negative temperatures,
  100% humidity, rain units, identity validation/injection rejection; local-day
  rain boundaries/gaps; ISO timestamps with offsets/fractions/leap dates and
  NWS warning priority.
  Daily view checks cover reset at midnight before another packet, missing or
  backward dates/times, stale coverage and configured-timezone 23/25-hour days.
- Live Atlas adapter: native/SI conversions, missing fields/zeros, integer
  exception flags, bounded rain values and model binding. The full Sunday
  capture parsed 24,075 packets: 8,083 accepted, 15,972 repeats, 20 flagged.
  This is decoder-output JSON validation, not an RF pulse/reception test.
- Five Python console checks include request-ID matching and explicit refusal
  to capture the physical setup view containing the access key.

`python -m unittest discover -s tools -p 'test_*.py' -v` passed all 12 tests,
including the seven existing PC archive/receiver lifecycle tests. The receiver
service itself was not started.

`tools/verify_setup_page.py` exercised the actual embedded HTML in headless
Brave with synthetic responses: tabs, all settings belonging to one form,
blank secret fields, SSID text handling, CSRF save, responsive phone layout,
and no JavaScript errors. Review images are private local capture artifacts.

## On-device checks

- 1,000 recorded packets over acknowledged USB: 339 accepted, 661 duplicates,
  no invalid packets or serial errors. A separate 300-packet daylight replay
  produced 100 accepted and 200 duplicates, Gust 5m = 20 mph, 56.3 °F and
  37,610 lux at the end of that recorded segment. All are explicitly replay;
  they were not uploaded or written to production weather checkpoints.
- Direct framebuffer review of normal/orange/red themes, Trends and Forecast.
  Palette previews were labeled THEME PREVIEW / REPLAY and then cleared.
  The page-switching banner is absent; the compact Home v2 layout remains.
- `bash tools/firmware-console.sh diagnostics` passed the actual LittleFS
  checkpoint test: write, replacement, round-trip history/counter/daily state,
  wrong sensor/time rejection and CRC corruption rejection. It uses temporary
  test files that are removed; production weather data is not replaced.
- On-device HTTP self-test passed: unauthorized config is rejected, authorized
  config masks secrets, missing CSRF is rejected, and out-of-range sensor IDs
  are rejected before narrowing. It performs no configuration writes.
- Owner used the phone portal to save Wi-Fi. The station restarted, loaded its
  saved settings, connected at `192.168.0.101` and synchronized network time.
  Unauthenticated LAN requests to `/`, `/api/config`, `/api/status` returned
  HTTP 401. The owner then saved station coordinates for NWS validation.
- Configured NWS HTTPS calls fetched 14 day/night periods and 24 hourly
  forecasts. Framebuffer review confirmed the local forecast, issue/check
  times and a fresh complete no-active-alerts response.
- A controlled USB chip reset recovered saved Wi-Fi/location, network time,
  14 day/night periods and 24 hourly periods, with normal Home startup.
- With radio absent, display/setup/serial remain responsive. Typical free heap
  after startup is 216 KB, free PSRAM 1.844 MB; measured main-task stack
  headroom remained above 15 KB through replay, portal and diagnostic checks.
- Two stack overflows discovered during integration were corrected: large
  temporary settings documents moved to heap and main-task stack explicitly
  sized to 24 KiB. Subsequent checks above passed. Native USB flashing during
  the rapid initial reset loop required a physical unplug/reconnect.

Records: ignored `captures/firmware-validation/` contains host/Python results,
USB replay results, runtime/diagnostic JSON, setup browser views and panel
framebuffers. No setup key, Wi-Fi password or upload credential is included.
The actual setup view refused a framebuffer request while the owner used it.

## Home v2 visual review after resuming

Compared `concepts/home-screen-v2.png` and its editable SVG with framebuffer
captures from the actual 320×240 board. Updated proportional DejaVu type,
baseline positions, theme colors, card dimensions, centered dots, degree mark,
wind decimal and dynamic unit positioning. Home now follows the study's visual
hierarchy; [intentional text changes](../HOME_SCREEN.md#home-v2-visual-comparison--october-20-2026)
retain later owner requirements and unavailable-hardware labels. This is a
visual comparison, not a claim of pixel-for-pixel identity with fictional data.

Reviewed labeled replay previews of clear, watch and warning states, including
the official-event line and check age, plus 72.4 °F / 8.2 mph / 58% examples.
An extreme-value check used 158.0 °F, 200.0 mph and 100% humidity: smaller
number fonts and measured unit spacing keep all values within their areas.
Shared header/alert strip were also checked on Trends and Forecast. USB replay
previews affect only the displayed snapshot, not NWS caches or live exports.
Replay/preview were cleared afterward and the board returned to Home.

The new build preserved configured Wi-Fi/location. With the radio disconnected,
NWS recovered 14 day/night and 24 hourly periods plus fresh alert status.
After downloaded sunset, Home selected the phase-rendered moon using fresh
NWS conditions; live weather measurements correctly remained unavailable.
The owner independently confirmed the sunset-to-moon switch on the panel.
Host model/replay checks and all 12 Python checks passed. The firmware compiled,
uploaded and verified successfully. The pre-update runtime had stayed running
for about 4.5 hours; this is useful network-only evidence, not a multiday or
simultaneous-RF soak result.

Local artifacts: `home-before-review.png`, `home-v2-final-theme-{0,2,3}.png`,
`home-v2-extreme-values.png`, `home-v2-shared-page-{1,2}.png`,
`home-v2-final-live.png` and `home-v2-final-runtime.json` under the ignored
validation directory. Theme previews explicitly say `THEME PREVIEW` / `REPLAY`.

Repeated framebuffer export exposed a short USB write: the initial exporter
ignored the returned byte count and could lose pixels. It now retries the
unsent portion with a bounded wait; eight consecutive complete frame transfers
passed afterward. Flash diagnostics also exceeded the host's ordinary
five-second reply budget once; the console now allows 15 seconds for that
command. Final on-device checkpoint and portal diagnostics passed, and Home
was left with replay/preview cleared, radio absent and Wi-Fi/NWS working.

## Local-day rain and measurement details

Latest owner request restores **RAIN TODAY** on Home and the portal overview.
It uses the configured local date, not a fixed 24-hour window. Counter and
rolling interval state are preserved; checkpoint schema 3 remains compatible.
No date/baseline shows unavailable. Stale or uncertain coverage cannot become
a complete daily upload merely because the display reset.

An on-board USB replay used the configured Eastern timezone and crossed
midnight: 0.02 in before, partial 0.00 after midnight with no additional
packet, then complete 0.00 after a dry packet confirmed continuity. The
rolling/session observed amount stayed at 0.02 in. Host tests additionally
cover 23-hour spring and 25-hour fall local days. The browser check supplies
different daily and rolling values to confirm the overview selects daily rain.

Captured Outdoor, Wind, Humidity, Rain and Pressure detail views on the actual
board. They expose field ages, UV/light, gust age/coverage and separate rain
totals; missing pressure remains unavailable. The same NWS strip/theme is
drawn on details. New physical tap/swipe/Back and alert-return targets still
need the owner's touch check; serial commands verified rendering only.

Code review also found that short uptime minute indices placed new history
at the left before clock synchronization. Sparkline and 24-hour chart now
anchor their right edge to the current minute, leaving earlier time blank.
An early-time USB replay verified the chart data at column 303, the rightmost
data column, on the board. This tests placement, not a full offline cold boot.

Host model checks, all 12 Python checks, browser checks, final build/upload,
and on-board checkpoint/portal diagnostics passed. The board is back on Home,
replay/preview/detail closed, Wi-Fi/time/NWS recovered, radio absent and rain
unavailable. Real radio reception and natural-midnight rain accumulation
remain hardware checks once the CC1101 is connected.

Artifacts under `captures/firmware-validation/`: `rain-today-midnight.json`,
`rain-today-midnight.png`, `measurement-detail-{0,1,2,3,4}.png`,
`pre-clock-{home,trends}.png`, `rain-today-final-home.png`,
`rain-today-final-runtime.json`, and `rain-today-final-diagnostics.json`.

## Remaining acceptance gates

- Connect the pictured CC1101 with regulated 3.3 V power using [WIRING.md](WIRING.md).
  Confirm SPI, compare live Atlas packet counts/values with the SDR reference,
  and test reception during TLS, plotting and flash checkpoints.
- Extend the successful live NWS fetch check to outages, updates/cancels,
  pagination and long texts. Normal forecast/empty-alert response is verified;
  production warning transitions and extended failure behavior remain open.
- Configure each intended sharing account and verify actual remote ingestion,
  registration and routing. Defaults remain off. APRS send status does not
  establish map visibility, CWOP collection or NOAA MADIS quality acceptance.
- Power-loss/restart recovery, clock/DST transitions, a full day of live
  rain coverage and a multiday soak. The filesystem test is not a physical
  power-cut/soak test. UTC-indexed history restoration needs valid clock time.
- Windy, optional D710 electrical/serial integration, calibrated sky estimate,
  barometer and optional sound are follow-up work. Measurement details are
  implemented; owner touch/gesture validation remains open.

Owner's final navigation update: horizontal swipes wrap in both directions
between Home, Trends and Forecast. The final build includes this change; actual
finger swipe behavior should be confirmed on the panel after upload.

## Final owner additions

- Normal NWS alert interval reduced to 30 seconds, while honoring longer
  server cache/retry requirements. Forecast refresh remains 30 minutes.
- An earlier main/setup rain revision used rolling 24 hours; the latest owner
  request supersedes it with RAIN TODAY (see above). Bounded 4,096-entry rain-counter
  intervals are stored in PSRAM and CRC checkpoint schema 3. Host checks cover
  first-baseline partial windows, 24-hour coverage, exact expiration, ambiguous
  boundary rainfall and gaps; endpoint ambiguity is explicitly marked partial.
- NWS current hourly forecast now supplies the labeled main icon. Icons are
  hidden without Wi-Fi/fresh NWS. Packed original project PNGs supply weather
  conditions; vector moon shading uses downloaded phase and illumination.
- Sunrise-Sunset.org v2 supplies daily sun times and moon information in one
  cached HTTPS response. Required attribution appears in setup/portal.
- ZIP lookup → embedded map → confirmed pin and ground elevation are now
  implemented and uploaded in the map/security build, with manual entry preserved.

Final live check: sunrise/sunset and moon data successfully downloaded on the
ESP32 over verified HTTPS after using the service's trusted GTS Root R4 CA.
The service returned First Quarter / approximately 68.77% illumination for
October 20. The final status has `sun_state: Current`, 14/24 NWS periods,
`alerts_valid: true` and `alert_poll_ms: 30000`. Alert check timestamps observed
across a normal cycle differed by 31 seconds, including request time. Final
free heap was approximately 205 KB and free PSRAM 1.734 MB, with over 15 KB
main-task stack headroom. Board left on Home; replay/theme preview cleared.
The owner subsequently confirmed the actual sunset-to-moon change. Later-date
phase transitions and full live 24-hour rain coverage remain longer checks.


## Map, admin privacy and v0.3.0

The v0.2.0 map/security image (1,488,705 application bytes) was uploaded while
preserving saved Wi-Fi/location/custom admin credentials. Its actual local
portal self-test passed, including masked settings output and rejection of
the visible AP key as the custom admin credential. Runtime reports both
`custom_admin_password` and `setup_credentials_separated` true. Physical setup
screenshots remain blocked. Owner phone map testing is still requested.

Browser checks of the actual embedded HTML and local Leaflet passed: lazy
provider requests, ZIP+4, centering without silently changing coordinates,
explicit point confirmation, metres/feet, invalid/missing elevation, manual
fallback, safe place text, phone layout and preserved sharing/identity fields.
All map tiles/provider replies in browser automation are mocked. Separate
public ZIP and USGS HTTPS requests returned successful CORS-capable responses;
that does not prove the owner's phone/provider/network combination. Save and
update controls refresh CSRF after a device restart without discarding edits.

The v0.3.0 image compiles and was uploaded over USB. Host checks pass for update repository/version/
URL/digest policy, sensor current/stale/silent transitions, credential migration
and strict APRS logresp parsing, alongside the existing weather/report/adapter
checks. Both setup and map browser suites pass. Local synthetic-repository
release packaging produces the expected image/manifest and asset notices;
nothing is published. GitHub hosted download, interrupted install, failed-boot
rollback, simultaneous TLS/RF load and real APRS session/ingestion remain open.


After the v0.3.0 upload, the real local portal and checkpoint diagnostics pass.
Portal checks exercise unauthorized update access, gzip map asset delivery,
custom-admin/AP-key separation and refusal of update installation without CSRF.
Display rendering was captured on Home and Link status. Wi-Fi/time recovered;
NWS has 14 forecast periods, 24 hourly periods and valid alerts at the configured
30-second interval. Stable free heap is about 186.8 KB, free PSRAM 1.778 MB and
main-loop stack headroom over 15 KB; transient NWS TLS use lowers free heap.
This is brief runtime evidence, not a simultaneous RF/OTA-load soak.

A short on-device USB replay sent a full 37/38/39 cycle, verified Current, then
continued only type 37 across 160 seconds of replay time. The display/status
changed to Data stale with types 38/39 missing. This is a model/UI integration
test, not a live RF freeze test. Replay was cleared immediately; final Home has
source none, No receiver, no readings, no preview/detail and no serial errors.
Both automatic update options remain off; nothing has been downloaded/published.

Latest private evidence: `v030-{home,link,stale-replay}.png`,
`v030-health-replay.json`, `v030-diagnostics.json` and
`v030-final-runtime.json` under `captures/firmware-validation/`.


## v0.3.1 setup-save and header correction

Reproduced the reported second-save failure in a browser regression: after
one successful settings POST the page kept its Save button disabled permanently.
The handler now releases its busy/button state after either success or error;
existing CSRF refresh allows another save after restart without page reload.
Native validation of a hidden tab could also prevent the submit handler from
running. Explicit validation now reveals the invalid field's tab and displays
its label/error before requesting correction. Ground elevation accepts arbitrary
precision, avoiding step mismatches from persisted floating-point values.

The browser regression now saves twice from Sharing without reloading and
verifies both payloads, accepts a saved fractional elevation, then clears the
Sensor ID on another tab and confirms Station opens with a visible error and
no unintended POST. Both setup and map browser suites pass. The firmware build
passes (1,517,237 application bytes, 68,212 static RAM bytes).

The callsign uses the existing 11-pixel bold header font instead of the 8-pixel
status font. RF/Wi-Fi/APRS/CWOP indicators use a separating space, with X for
disconnected/off and + for connected. Link details retain disabled/error reasons.
Owner's real APRS connection was confirmed true before uploading this patch;
this establishes session login, not live weather ingestion.


USB upload of v0.3.1 passed with saved configuration preserved. Actual local
portal self-test, display initialization, credential separation, Wi-Fi/time
and NWS alerts pass after restart. Native Home capture confirms the enlarged
saved callsign and spaced indicators. Latest private evidence is
`v031-home.png` and `v031-runtime.json` under the validation captures directory.
The owner must refresh the existing setup page to load the corrected JavaScript;
previously cached/open tabs still contain the disabled-button bug.


## First physical CC1101 reception and v0.3.2 UV correction

Owner connected the pictured CC1101. The first v0.3.1 live status confirms
radio initialization, source radio, Atlas packets accepted/repeated, no queue
drops and zero rejected packets. Home shows 45.4 F, 94% humidity, zero wind,
south direction and zero lux at night. These are received readings; external
reference accuracy and supply voltage under load remain unmeasured.

Initial reception exposed a native field mismatch: the pinned embedded
acurite.c emits `uv`, while our adapter only read the desktop `uvi` alias.
UV remained missing, preventing Current health and therefore blocking weather
uploads. A new regression reproduced this failure before the fix. v0.3.2 reads
native `uv` first with `uvi` fallback, preserving zero and missing distinctions;
the PC replay adapter accepts both too. A native 37/38/39 fixture cycle now
reaches Current through the real adapter/store/health path.

Host firmware checks and all 13 Python tests pass. The pinned build passes
at 1,517,265 application bytes and 68,212 static RAM bytes. This corrects a
mapping error, not the health/upload eligibility rules. No replay is needed
on the live receiver to validate this change.


After USB upload, a 180.4-second live observation on v0.3.2 reached Current by
45 seconds uptime and retained it through the remaining samples. Final counters:
18 accepted frames, 33 repeats, zero rejected frames, zero decoder queue drops
and zero serial errors. Wi-Fi/NTP, 14/24 NWS products, fresh 30-second alerts,
portal self-test and the verified APRS connection remain healthy. Free heap
was approximately 150–152 KB and main-loop stack headroom exceeded 15 KB.
A screenshot shows real 45.4 F, 95% RH, 2 mph SSW and zero nighttime lux.
Link details show Current, no missing packet types, a verified APRS session
and a last-report age. This confirms a socket send, not remote ingestion.
The daily rain view is zero with complete coverage after the local date change;
this is a dry boundary observation, not a test of rainfall across midnight.

Private evidence: `first-live-home.png` (before UV correction),
`first-live-v032-monitor.json`, `first-live-v032-home.png` and
`first-live-v032-link.png` under captures/firmware-validation/. No replay or
artificial weather was injected. Physical supply current/voltage, SDR reference
comparison, simultaneous OTA/RF operation and a full soak remain unverified.


## Owner-confirmed APRS.fi and Weather Underground ingestion — October 21

The owner confirmed the live weather packet appears at the correct location
on APRS.fi with the CYDWeatherLink identifier, and Weather Underground shows
the uploaded data. This establishes observed remote display/ingestion for
these configured accounts; it does not verify CWOP registration/MADIS ingestion,
Weathercloud, all measurement encodings or extended outage recovery.

Normal APRS weather interval is 300,000 ms (five minutes) after a successful
socket write, while maintaining the logged-in connection. Failed sends back
off up to one hour. WU normally uploads every 60 seconds after acceptance,
with failure backoff up to 30 minutes. Both remain gated on fresh live station
data and valid time; request duration/scheduling can lengthen these intervals.


## Seven-day archive — v0.4.0, October 21

Kept the existing 1,440-minute display ring and checkpoint schema 3. Added
2,016 fixed 64-byte five-minute records (129,024 bytes) in internal LittleFS,
CRC/bound-station checks, coverage flags, observed rain increments, temperature
mean/min/max, humidity mean, wind mean/peak, light mean/peak and reception counts.
Archive allocation/commits preserve a 512 KiB free-space reserve. No partition,
NVS secret, radio pin, uploader cadence or live-data gate changed.

Host archive tests pass for fixed layout, means/peaks, genuine zero versus
missing, elapsed coverage with normal 1.01-second loop jitter, actual pauses,
backwards time, seven-day expiry, CRC, binding, ambiguous rain boundaries,
partial restart merging, rejected/other-station frames and count saturation.
All existing firmware checks and 16 Python tests pass. Browser setup/map checks
pass, including CSV page assembly and a failed-page retry. The pinned build
uses 1,535,501 application bytes and 68,276 static RAM bytes (45.9% / 20.8%).

USB upload restored the existing real minute-history checkpoint. The archive
allocated successfully and the actual protected portal self-test passes,
including unauthorized archive/CSV rejection and authenticated archive metadata.
On-device checkpoint and archive read/replacement/CRC diagnostics passed.
Native USB commands now synchronize with an acknowledged status and request
exclusive serial access; diagnostic reply timeout is 45 seconds. Only one
process may use the port. These are host transport fixes, not station resets.

Exported real CSV through the paged USB path. Saved three real records, updated/
restarted the board, then exported again: every saved row matched field for
field. Filesystem free space is 1,298,432 bytes (about 1.24 MiB), well above
the reserve; archive write failures remain zero. Live Atlas reception and the
APRS connection recover after startup, with no replay or artificial weather
injected. A second save/export confirmed the resumed interval continued adding
live samples while older completed rows stayed identical. Current partial
intervals deliberately retain restart/rain-gap flags.

Private evidence: `archive-before-runtime.json`, `archive-v040-runtime.json`,
`archive-v040-diagnostics.json`, `archive-v040-before-reboot-save.json`,
`archive-v040-{first,before-reboot,after-reboot}.csv`,
`archive-v040-after-reboot.json` and `archive-v040-final-runtime.json` under
captures/firmware-validation/. Browser fixtures/screenshots there are synthetic.

Actual on-board browser CSV downloading still needs the owner's phone check;
USB uses the same CSV-page formatter and browser assembly was tested headlessly.
A physical power interruption during a slot write and full seven-day runtime/
rollover remain soak acceptance checks. See RECORDING.md for retention, loss
bounds, UTC, precision, storage reserve and export semantics. The PC receiver
logger remains stopped and no cloud archive upload was added.
