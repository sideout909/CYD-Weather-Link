# PanelLan weather firmware

Current stable release: **0.8.2**, fixing OTA startup settings validation while
retaining the owner-verified Windy TLS fix. See [patch notes](RELEASE_0.8.2.md)
and [Windy fix](WINDY_TLS_FIX.md); 0.8.0 introduced the features below.
See [release notes](RELEASE_0.8.0.md) for factory reset, units, configurable
hostnames, new sharing destinations, pressure fallback and experimental local
cloud estimation. The owner can update the installed candidate through OTA.

October 22, 2026 · release version 0.7.5 · owned PanelLan
`ZX2D80CE02S-V10` / `SC05_X`. This profile is specific to its parallel ST7789
LCD and FT5x06 touch, 8 MB flash and 2 MB PSRAM. Do not flash a generic CYD or
SC01 Plus with this profile. See [validation](VALIDATION.md) for the distinction
between implemented software, bench results, and remaining hardware checks.

## Core implemented

- Home v2 layout: compact header, temperature/sparkline, compass, received wind,
  five-minute maximum, humidity/rain/pressure tiles and persistent NWS strip.
  Three swipe pages, no page-switching banner. BARO uses a fresh nearby NWS
  pressure observation when available; the Atlas has no barometer. The main icon uses a current NWS hourly forecast,
  labels that source, and hides offline/stale; local lux stays independent.
  DejaVu typography, native-size geometry and palette follow the v2 study;
  [intentional wording differences](../HOME_SCREEN.md#home-v2-visual-comparison--october-20-2026)
  track later owner requests and hardware availability.
  Measurement detail taps expose field ages, local light/UV, wind peak age
  and coverage, rain intervals and pressure source/type/age.
- CC1101 receive-only 433.92 MHz input using pinned RadioLib/rtl_433_ESP,
  Acurite decoder selection and an Atlas-only application filter. Decoder task
  copies into a bounded queue; weather/UI updates happen on the main task.
  Missing radio does not prevent the display or setup portal from operating.
  [Exact connector wiring for the owner's photo](WIRING.md).
- Configurable sensor ID/channel (679/A by default), separate field ages,
  repeat grouping, real zero values, invalid/exception rejection, exact
  five-minute gust expiry and incomplete-window marking. Live decoder supports
  native and SI units. The display's Gust 5m is a maximum received sample,
  not a claim of a measured standard three-second gust or sustained wind.
- 1,440 minute buckets in PSRAM, temperature/humidity/lux means and wind peaks,
  gap-preserving detailed charts and bounded internal-flash checkpoints every five
  minutes. CRC/schema/station/time checks gate restoring weather checkpoints.
- [Seven-day recording](RECORDING.md): fixed five-minute summaries, 126 KiB
  logical ring, CRC/coverage flags, protected browser and paged USB CSV export.
  It preserves the 24-hour charts and a 512 KiB filesystem reserve.
- Atlas rain increments, initial counter baseline, reset/wrap uncertainty,
  RAIN TODAY main tile with local-midnight reset plus separate rolling hour/24-hour totals. Rolling totals become eligible for interval uploads only with full window
  coverage; calendar-day uploads require uninterrupted midnight/day coverage. Counter
  and daily state plus a bounded interval ring are checkpointed; replay never
  overwrites live checkpoints.
- Protected setup Wi-Fi, local DNS, authenticated responsive setup webpage,
  network scanning, sensor/time/location/sharing settings, NVS persistence,
  blank-secret retention, CSRF protection and sanitized health responses.
  The setup Wi-Fi key is generated separately from a user-chosen admin
  password; custom admin passwords are never drawn on the display. Embedded
  ZIP/map selection includes explicit point confirmation and USGS ground
  elevation, with manual entry retained offline.
- Wi-Fi reconnect, NTP, configured timezone, monotonic observation ages, and
  explicit recorded USB replay that cannot become a live upload source.
- NWS linked day/night periods and next 24 hourly periods, active alerts,
  30-second normal alert polling, priority ordering, warning/watch strip,
  scrollable details and cached
  forecasts/alerts. Verified HTTPS, bounded background work, rate/backoff,
  cache/retry headers, and separate stale/unverified status. Internet failures
  do not block local readings; cached alerts cannot establish an all-clear.
- Independently opt-in Weather Underground (60 s), Weathercloud (10 min), and
  APRS-IS/CWOP (5 min) exporters with credentials, destination-specific units,
  fresh-live-only gating and backoff. No pressure or solar irradiance is
  invented. APRS sustained wind and unsupported rain windows are omitted.
  A TCP send is shown as sent, **not proof of remote ingestion**.
- Callsign header slot and right-aligned RF/WiFi/APRS/CWOP status group. The
  Internet session stays open with reconnect/backoff and TCP keepalive;
  ham login must be verified. A separate Atlas monitor flags missing cycles,
  ten-minute missing/unchanged data warnings and a five-minute packet silence
  (`Disconnected`). Unchanged data is a warning heuristic, not proof of failure.
  Display field freshness is ten minutes; uploads and archive completeness retain
  their separate two-minute complete-cycle checks.
- [Opt-in GitHub release updates](UPDATES.md): daily checks, manual install
  or explicit automatic installation, compatible-image/SHA-256 checks, inactive
  slot writes, pre-restart weather checkpoint and deferred startup confirmation
  for rollback. Real hosted download/rollback acceptance remains open.

Live CC1101 reception and owner-observed APRS.fi, Weather Underground and
Weathercloud ingestion are confirmed. SDR comparison, separate CWOP ingestion,
extended reception during TLS traffic, abrupt-power-loss testing and a multiday
soak remain acceptance gates.
Windy (third PWS adapter) remains follow-up work. D710 electrical/serial
integration and barometer are optional additions. Sound is removed from this hardware version. SD is optional, and the PC RTL-SDR
capture service remains stopped.

## Setup and controls

Open http://wxlink.local from the setup Wi-Fi or same home LAN. Tap the
display title or Wi-Fi indicator to see this address and its numeric IP fallback. Before
setup, this shows the protected `WXLink-…` network, generated access key and URL.
After home Wi-Fi and station confirmation are saved, it shows the current URL
and admin guidance; use the recovery/details button for setup-network details. Join on a phone, open `http://wxlink.local` (fallback `http://192.168.4.1`),
and authenticate as `admin`
with the initial displayed Wi-Fi key, or your chosen admin password after
changing it. Save your home Wi-Fi, location, timezone and sensor
binding. The display's setup view also shows the current LAN address.
Passwords/keys are not returned in configuration or serial status; leave
secret fields blank to retain saved values. Do not post keys in chat or logs.
The displayed setup Wi-Fi key and custom admin password are independent.
Changing the admin password leaves the Wi-Fi key unchanged, unless the chosen
password equals that visible key (then the Wi-Fi key is regenerated). Legacy
shared-key configurations retain their admin password privately and receive
a new generated Wi-Fi key once. API responses never return either key.
The setup network remains available if home Wi-Fi cannot connect. It uses
WPA2; the local setup HTTP page itself does not provide HTTPS.

Swipe horizontally in the page body to cycle Home/Trends/Forecast. Navigation
wraps in both directions: Forecast → Home and Home → Forecast. Tap the
trend chart to change metric. On Forecast, tap its daily/hourly label to switch
products, tap the body for the next period, and drag vertically to scroll.
Tap the bottom NWS strip for alert details, drag to scroll, tap the bottom to
advance alerts, and tap its header to return. Tap the setup header to close.

On Home, tap the outdoor/wind panels or humidity/rain/pressure tiles for
measurement details. Tap the 44-pixel-high Back header to return Home. Swipe
an outdoor, wind or humidity detail to open that metric's 24-hour trend.
Rain details show Today, Last hour and Last 24 hours separately, counter age
and partial-coverage marking. The NWS strip and watch/warning theme remain
visible in measurement details; returning from alert text restores the detail.
For USB bench review, `bash tools/firmware-console.sh detail rain` opens a
detail without touch; choices are `outdoor`, `wind`, `humidity`, `rain` and
`pressure`. `page home` closes it. This does not inject weather data.
`detail link` opens connection details; on the panel, tap the RF or APRS/CWOP part of the second header
row (the Wi-Fi target opens settings). RF + is receiver readiness, Wi-Fi + is association, and APRS + / CWOP +
is an acknowledged session. X means disconnected or disabled; Link status
provides the reason. Each label has a space before its indicator. Field ages and
station health remain independent of those links. There is no sound indicator
or sound function on this hardware version.

ZIP/map/elevation is inside Station & Wi-Fi. ZIP search only centers the map;
tap/drop or drag a pin, review its USGS ground estimate, then Use this location
to copy it into the form. Save and restart persists it. Manual entry works
without map services; an unavailable elevation requires a manual value rather
than silently retaining the old site's elevation. Timezone stays explicit.
[Map sources and privacy](../assets/web/README.md) explain the browser-only
lookups and attribution. Update controls have a separate Updates tab.

The **RAIN TODAY** tile sums receiver-derived increments for the configured
station's local date and resets at local midnight, including daylight-saving
days. An existing baseline permits a partial `0.00` immediately after midnight
before another rain packet arrives. A missing baseline or unknown date shows
`--`. The previous packet's baseline remains intact for continuity checks;
the sensor's raw counter is never reset. The first day and gaps remain partial,
and nonzero increments straddling midnight make day allocation uncertain.
`*` means partial/uncertain coverage and `!` marks stale counter reception.
The first station counter becomes a baseline, never today's rain. Separate
hour/24-hour rolling totals remain in details and eligible exports. Exact
counter intervals and daily state persist with history (schema 3). A rolling interval that
straddles the left boundary has uncertain tip timing: those tips are excluded
and the window is marked partial. Gaps/reset/wrap uncertainty also prevent a
complete label. Rolling 24-hour coverage normally takes 24 hours from the first baseline.
Hour/24-hour interval exports are eligible only with complete coverage;
calendar-day rain remains separate for services requiring midnight totals.

NWS hourly weather selects the main icon, explicitly a forecast. Sun data by
[Sunrise-Sunset.org](https://sunrise-sunset.org/api) provides sunrise/sunset,
moon phase and illumination for the configured local date over verified HTTPS.
One daily request supplies all four; the result is cached. Sun times select
day/night; NWS day/night metadata is the fallback. At night the moon graphic
uses phase/illumination and waxing/waning, mirrored for the southern hemisphere.
Missing phase data stays unknown. Sun/moon details and attribution are in setup.
The owner confirmed the first sunset-to-moon change on the physical display;
changes across later dates and lunar phases remain longer-term checks.

At most five minutes of summaries may be lost on sudden power-off. Restored
checkpoints do not restore a fresh gust. UTC-indexed restoration requires valid
time; incomplete capture intervals remain visible as partial coverage.

## Build and test

Run from the repository root on Linux with Python 3, Git, and `g++`:

```bash
bash tools/setup-firmware.sh
bash tools/pio.sh run -d firmware
bash tools/test-firmware.sh
```

The tools use a private environment under
`${XDG_DATA_HOME:-$HOME/.local/share}/cyd-weather-link/venv`, outside the project
path because a PlatformIO dependency installer mishandles Python paths with
spaces. `tools/firmware-requirements.txt` pins PlatformIO/esptool. The PlatformIO
profile pins Espressif32 6.12.0 (Arduino 2.0.17), LovyanGFX 1.2.7, ArduinoJson
6.21.5, and the manufacturer's library commit. No global Python installation
or root access is needed for builds.

Custom hardware manifest: `boards/panellan_sc05_x.json`, with 8 MB QIO flash,
QSPI PSRAM, native USB Serial/JTAG, and 240 MHz CPU. Pin assignments for the
display/touch come from the vendor library, not an assumed DevKit pin map.
The build uses `default_8MB.csv`: two application slots and a bounded internal
LittleFS partition. Settings use NVS. Opt-in GitHub application updates use the
inactive slot; see [UPDATES.md](UPDATES.md) for packaging and pending OTA bench checks.

## Upload and console

First install permissions if needed using `PARTS.md`. Close any serial monitor
before uploading. The existing firmware was backed up locally under ignored
`captures/hardware-backups/`; see the matching manifest for its checksum. Full
flash backups may contain configuration and must not be published.

```bash
bash tools/pio.sh run -d firmware -t upload --upload-port /dev/ttyACM0
bash tools/firmware-console.sh status
bash tools/firmware-console.sh diagnostics
bash tools/firmware-console.sh page trends
bash tools/firmware-console.sh page home
bash tools/firmware-console.sh screen captures/firmware-home.png
```

The console preserves firmware state across connections by holding native
USB DTR/RTS asserted together; deasserting both caused a USB chip reset on
this board during bring-up. The console defaults to this board's stable `/dev/serial/by-id/` path and also
accepts `--port` before the subcommand. Only one tool should hold the port.

Replay captured JSONL (or `.jsonl.gz`) explicitly:

```bash
bash tools/firmware-console.sh replay captures/atlas/packets-2026-10-18.jsonl --limit 25000
```

Replay clears prior in-memory weather/history, retains source timestamps,
waits for each packet acknowledgement, and filters the bound station. It
does not start the SDR or upload observations. Use `--start` and `--end` with
ISO-8601 UTC timestamps to restrict a capture. Logical replay time follows
the latest source timestamp and then advances with monotonic elapsed time;
it is not presented as the current wall clock. The replay display uses
the configured POSIX timezone (Detroit by default). A new replay resets this clock.

## Development protocol

One JSON object per line, maximum 1,023 bytes. Main-loop serial consumption
and rendering are bounded. Operations: `status`, `diagnostics`, `reset`, `page` (0–2),
`sample`, `screen`, `archive` and `checkpoint_save`. `archive` pages the
seven-day CSV; `checkpoint_save` saves only live history and its partial archive
interval. The host CLI uses `archive OUTPUT.csv` and `checkpoint-save`. `sample` is a USB replay input, not a production radio
interface. Its fields are defined by `tools/firmware_console.py`.

Sample acknowledgement results: 0 accepted, 1 duplicate, 2 wrong station,
3 exception flagged, 4 invalid value/time, 5 backwards timestamp. Repeats do
not refresh field ages or extend gust peaks. A peak expires on the open left
edge `(now - 300000 ms, now]`. The fixed 512-sample wind buffer marks overflow
as incomplete coverage. An overlong line is discarded through its newline.

`screen` returns `SCREEN RGB888 320 240 230400`, then 230,400 RGB bytes and
a newline. The host converts this to PNG without another imaging dependency.
Framebuffer capture proves what was rendered; physical panel appearance and
touch alignment still need observation on the actual board.

## Display, clock and packet-age preferences — v0.7.1

Station & Wi-Fi includes a shared color theme for the display and webpage, date format, a 24-hour clock toggle, and display
inactivity behavior. Keep the screen on is the default. Dim uses 10% of normal
brightness (Night uses 24/255 normally and 2/255 dimmed); Off switches off the backlight. Touch wakes without activating a
control. Reception, recording, NWS and all uploaders continue. Trends/Forecast
return to Home after 60 seconds without touch.

The header centers a larger callsign, shows date before time, and groups status
indicators toward the right. NWS is above its icon; UV value and full level are
beside klux. Forecast pages use original project icons, with moon overlays for
known night conditions including clouds, fog and precipitation. Receiving means
packets are arriving but a complete current cycle is not yet available.
The source row also shows seconds since the last decoded bound-station packet,
including valid repeats (for example `Atlas · Current · 12s`). This is reception
age, separate from per-field freshness; no age appears before first reception.

After setup, the display directs users to the settings URL/admin login; custom
passwords stay hidden. Wi-Fi recovery details remain available deliberately.

## Friendly local address — v0.7.3

The built-in mDNS responder advertises wxlink.local and HTTP on TCP port 80,
using existing AP/STA interfaces. First-run and More settings views show
http://wxlink.local plus numeric IP fallback. The browser header repeats both;
status/USB reports include local_url and mdns_ready. Failed responder startup
retries every 30 seconds without making radio/internet a boot requirement.
Normal ESP-IDF interface events handle address/reconnect changes. The internal startup probe uses SDK loopback so AP/STA route changes do not
prevent testing the actual protected HTTP server. No secrets
are advertised, and the page/API authentication and CSRF checks are unchanged.
Local multicast/name-resolution support is required; the IP always works when
reachable. The default hostname assumes one device per LAN; configurable
names for multiple units are separate follow-up work.

## Shared color palettes

Color theme offers Midnight (original), Matrix, Amber, Ice Blue, Violet and
Paper, plus Night (low light). Selection previews the browser only; Save and restart persists it for
both screen and browser. Old settings keep Midnight. Normal geometry/controls
are unchanged; RF, WiFi and APRS/CWOP shift right into the former sound space.
Sound is removed, with earlier plans archived for future larger-display hardware.
The display's orange/red alert overrides and browser warning cards remain
contrasting across all palettes. Paper uses a darker orange for stale values. Night uses subdued amber/olive
colors and lower backlight, including softened NWS all-clear text and tinted
moon artwork. It wakes at its low brightness without a normal-brightness flash.
Forecast moons now estimate each period's phase from current provider data;
missing lunar data uses an unknown marker, not a fixed crescent.

`assets/themes/palettes.json` is shared by generated `display_themes.h`, CSS
and `theme_icons.h`. Run `python tools/pack-display-themes.py` after palette or
original SVG changes, then `python tools/pack-setup-assets.py` after browser edits.
Generation needs `rsvg-convert`; ordinary builds use checked-in generated data.

## Validation record

Version 0.5.1 adds automatic Atlas discovery in Station & Wi-Fi. Scanning can
take up to five minutes; the user confirms live readings without typing an ID,
then saves settings. Existing bindings keep receiving while scanning. Iris and
Notos appear as detection previews with integration pending. New devices stay
unbound until confirmed; existing settings migrate automatically. See
[decoder/discovery details](../DECODER_SUPPORT.md) and the
[short owner checklist](CONTROLS_CHECKLIST.md#discovery-checks-from-v051).

See [VALIDATION.md](VALIDATION.md) for measured build and bench results.
The host tests cover sparse fields/ages, zeros, station binding, exception and
NaN rejection, backwards time, duplicate peak age, exact expiry, startup/gap
coverage, rain baseline/wrap/reset, and history-ring overwrite. They do not
establish RF range, network behavior, or D710 compatibility.

Vendor sources:
[board library](https://github.com/smartpanle/PanelLan_esp32_arduino/tree/2bc0d086c9a55f5288d85da6c16968c22744fe5f),
[display/touch pins](https://github.com/smartpanle/PanelLan_esp32_arduino/blob/2bc0d086c9a55f5288d85da6c16968c22744fe5f/src/board/sc05_x/sc05x_pin.h).

Alert polling normally runs every 30 seconds, separate from 30-minute forecast
refreshes. Longer NWS cache or retry requirements and failure backoff take
precedence. This matches the [NWS alert service recommendation](https://www.weather.gov/documentation/services-web-alerts)
to request updates no more frequently than every 30 seconds. Health responses
include the last successful check and effective poll interval.

Original weather PNGs can be repacked after an asset change with
`python tools/pack-weather-icons.py`; the checked-in header is reproducible.

Home typography is packed in `include/home_fonts.h`. Regenerate with
`python tools/pack-home-fonts.py --font-dir /usr/share/fonts/TTF` using Pillow
and DejaVu Sans Book/Bold (validated with Pillow 12.3.0 and DejaVu 2.37).
Other systems can pass their own DejaVu font directory. Normal firmware builds
use the checked-in header and require neither Pillow nor desktop font files.
The packed glyphs retain 4-bit coverage for antialiased drawing over each theme.
The [DejaVu license](../assets/fonts/LICENSE-DejaVu.txt) accompanies the font
assets; include that notice when distributing the firmware.

The USB-only `theme_preview` command is accepted only during explicit replay.
Themes `0`, `2`, and `3` present fictional clear/watch/warning display snapshots
under a `THEME PREVIEW` / `REPLAY` header. They leave service caches and live
upload state untouched. `reset` clears both replay and preview; return Home
after visual checks. This facility is for bench review, not alert simulation
on the NWS service or any sharing network.

### October 21 follow-up: retained history and owner status policy

Version 0.4.7 combines the owner-confirmed ten-minute unchanged/incomplete-data
warning and five-minute disconnected status with the restored-history display
fix. Uploads and Current archive coverage still require a complete live cycle
with fields younger than two minutes, and Data stale suppresses uploads. The
large Home temperature is orange for retained stale/disconnected data; Home
omits the small Data stale status text and temperature STALE tag. Explicit
Disconnected and measurement-detail explanations remain available.

Host checks cover exact ten/five-minute boundaries, repeated unchanged reports,
recovery, strict upload completeness and restored buckets with zero accepted
live reports. The firmware build passes. The owner's board preserved and
rendered saved history after restart before new RF data arrived; see
RECORDING.md for the measured counts and archive comparison. RF follow-up is in
RADIO_DIAGNOSTICS.md. Timed live stale/disconnected appearance and a multiday
soak remain bench checks; an actual post-update OTA boot is still unverified.

### Local phone dashboard, v0.5.0

Overview now mirrors the main readings/status, followed by 24-hour trends with
real gaps, full day/night and hourly NWS forecasts, and expandable alert details.
New devices open setup first. See [WEB_DASHBOARD.md](WEB_DASHBOARD.md) for phone
access, formatting, source/freshness labels, refresh behavior and API bounds.
