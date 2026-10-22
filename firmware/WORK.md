# Current development state

Updated October 22, 2026 · firmware **v0.7.4**, installed over USB on the
owner's PanelLan ZX2D80CE02S-V10 / SC05_X (8 MB flash, 2 MB PSRAM). v0.7.4 is the USB-installed build. The v0.7.5 release adds the canonical
WX-Link update source and migration while retaining the validated UI.
The PC RTL-SDR capture service remains stopped. Device recording continues
independently of the PC. See [build and controls](README.md),
[bench evidence](VALIDATION.md) and [owner checklist](CONTROLS_CHECKLIST.md).

## v0.7.5 — public release preparation

The owner verified the v0.7.4 dashboard cleanup and reports exercising the
preceding firmware over the last 24 hours. v0.7.5 includes the renamed default
update source and migrates a saved old project source while preserving custom
forks. This version is newer than installed v0.7.4 so OTA can offer it. Build,
host checks, package integrity and public download verification are release gates;
hosted interruption/rollback validation and remaining hardware/service gates
remain open. Release preparation does not flash the owner's device.

## v0.7.4 — completed-setup dashboard cleanup

The Overview “Get connected” card now hides using the existing onboarding
completion rule (Wi-Fi configured, location set, station confirmed). Top tabs
remain available; incomplete setups retain guidance. Repacked browser assets.
The browser checks cover both states and retained top navigation, and the host
suite and SC05_X firmware build pass. USB upload succeeded; device status reports
v0.7.4, display/radio/Wi-Fi ready, restored checkpoint/archive and a passed portal
startup test. No GitHub Release or OTA asset was published.

## Implemented and observed

- Live Atlas reception through CC1101 on the [selected pin map](WIRING.md).
  Atlas is the only station enabled for operational readings, recording and
  uploads. Iris/Notos are discovery previews and cannot be bound yet.
- Protected Find my station flow: up to five minutes, repeated independent
  sightings, current measurements and explicit confirmation. Advanced manual
  binding remains available. Foreign discovery packets and replay cannot enter
  production weather history or uploads.
- Home v2-derived layout, larger centered callsign, compact right-aligned RF/WiFi/APRS
  group, configurable date and 12/24-hour time, temperature sparkline, received
  five-minute wind peak, rain today, light/UV level and attributed NWS pressure.
- Wrapping Home/Trends/Forecast swipes, field/link/alert details, gap-preserving
  24-hour trends and 60-second idle return from Trends/Forecast to Home.
  The small Home sparkline bridges missed minutes without inventing samples.
- NWS day/night and hourly forecasts, 30-second normal alert polling with
  cache/backoff, sunrise/sunset, phase-aware Home moon and cloud overlays.
  Forecast periods use original project icons and lunar-cycle phase estimates for each forecast period.
- Bound-station packet age in seconds beside the source state.
- Seven saved color palettes shared by screen/browser, with live web preview
  and recolored original weather artwork. Sound is removed from this hardware.
- Keep-on/dim/off backlight preferences. Touch wakes before controls activate;
  receiver, history, NWS and sharing continue while the screen is dark.
- Friendly http://wxlink.local mDNS address and numeric IP fallback on the
  display/browser; protected AP/LAN browser dashboard and setup, local ZIP/map tools with USGS
  ground elevation, explicit timezone/location, NVS settings and separate
  generated setup Wi-Fi key/custom admin password. Custom passwords are hidden.
- Bounded 24-hour minute history with five-minute CRC checkpoints, local-day
  rain, rolling rain intervals and a fixed seven-day/five-minute archive.
  Browser/USB CSV export and history restoration are bench observed.
- Opt-in APRS-IS/CWOP, Weather Underground and Weathercloud adapters. Owner
  confirms APRS.fi placement/identification and WU/Weathercloud ingestion.
  Normal intervals: APRS/CWOP five minutes, WU 60 seconds, Weathercloud ten
  minutes. Stale, incomplete and replay data cannot upload.
- Opt-in GitHub OTA, compatible inactive-slot images, SHA-256 checking and
  deferred startup confirmation. USB-installed startup self-tests pass.
  Corrected hosted OTA acceptance remains open after the earlier rollback.

The v0.6.2 follow-up renames Collecting to Receiving without changing health
policy and adds moon backgrounds to recognized night fog/precipitation/wind
conditions. Unknown forecast conditions keep their unknown icon.

The v0.6.3 dashboard correction distinguishes an unknown startup connection
from confirmed disconnection and retains status received before its deferred
script loads. Delayed-status and delayed-script browser regressions passed;
the pinned build uses 1,604,033 application bytes and 69,892 static RAM bytes.

## Latest v0.7.3 validation

Friendly address resolves on the owner LAN; unauthenticated HTTP through it
returns 401 and the owner confirmed browser access. After an AP-route probe
failure in v0.7.2, the full startup HTTP/auth/CSRF/schema checks now use SDK
loopback and pass on attempt one at 5.5 seconds uptime. Confirmation/rollback
policy is unchanged. USB installation restored 1,124 minute buckets and 241
archive records; RF resumed. Build: 1,698,173 app bytes (50.8%), 71,836 static
RAM bytes (21.9%). Settings views show hostname with numeric IP fallback.

## Previous v0.7.2 validation

Host and browser checks passed for local hostname/IP links and fallback
feedback. Build: 1,698,209 application bytes (50.8%) and 71,836 static RAM
bytes (21.9%). Protected startup checks validate hostname/readiness/IP schema.
Numeric addresses remain available without local multicast support. Final LAN
resolution and authentication results are recorded in VALIDATION.md.

## Previous v0.7.1 validation

Host and protected browser checks passed for the seventh Night palette,
softened NWS green, lower backlight/dim/wake policy and forecast lunar phase
progression. Build: 1,670,997 application bytes (50.0%) and 69,908 static RAM
bytes (21.3%). Current phase data anchors each forecast-period estimate; missing
lunar data stays unknown. No new provider requests or hardware parts.
USB installation restored the checkpoint, 1,113 minute buckets and 239 archive
records. Portal checks passed; Wi-Fi and RF recovered. The actual Forecast
capture shows the gibbous moon behind fog; saved palette was preserved.

## Previous v0.7.0 validation

Host tests and setup/discovery/location browser checks passed. Browser tests
exercise all six palettes, save/reload, unchanged readings and contrasting
Paper warning cards. Build: 1,650,017 application bytes (49.4%) and 69,908 static
RAM bytes (21.3%). USB startup checks passed with restored history (1,107
minute buckets, 237 archive records), ready radio and connected Wi-Fi. The
framebuffer confirms speaker removal and right-aligned statuses. Original
Midnight remains the migrated default. Physical
palette selection/persistence and shifted Wi-Fi touch need owner confirmation.

## Previous validation

The v0.6.2 host suite and protected setup/dashboard browser checks passed,
including Receiving wording and night fog moon/condition layers. Earlier
station-discovery and location-browser checks passed in v0.6.0. The v0.6.2
pinned build uses 1,603,857 application bytes (48.0% of the OTA slot) and
69,892 static RAM bytes (21.3%).

The v0.6.0 installation restored 222 archive records; its 65-second RF
observation confirmed automatic return to Home, Current station state and
zero capture/decoder/application queue drops. The v0.6.1 upload restored
1,088 minute buckets and 233 archive records with successful portal checks;
its framebuffer showed packet age without overlap. These are bounded bench
checks, not a completed multiday soak. The v0.6.2 USB installation restored 1,092 minute buckets and 234 archive
records; portal checks passed and Receiving appeared with live RF. The actual
night fog forecast capture shows its moon/condition layers. See
[VALIDATION.md](VALIDATION.md) for the dated evidence.

## Owner checks

- On the same Wi-Fi, open http://wxlink.local; confirm the admin login and
  normal dashboard. Verify both name and current IP on the device's settings
  view, then try the IP fallback. Repeat on setup Wi-Fi if needed.


- Try Night (low light): dark surfaces, subdued amber/olive text, softer NWS
  green, lower backlight and no bright flash on wake. Check Dim/Off and restart.
- Confirm forecast moons now follow phase, with matching cloud overlays on
  both display and webpage.


- Choose a theme in Station & Wi-Fi → Display & clock, preview it, then Save
  and restart. Confirm display/browser match after reload and reboot.
- Confirm RF/WiFi/APRS are shifted right and sound is absent; check the Wi-Fi
  touch target and retained stale/alert colors, especially on Paper.


The owner reports all previous v0.6.0 display/control changes passed their
checks. The new packet-age/Receiving label and night fog/precipitation artwork
need an owner visual check. The
original priority checks were:

1. Try Dim and Off with a 30-second timeout. Confirm the first touch only wakes,
   subsequent controls work, and reception/recording/sharing continue.
2. Check date formats and 24-hour time on display/browser and after restart.
3. Check the post-setup settings URL/admin guidance and deliberate recovery
   view; a custom admin password must remain hidden.
4. Complete the remaining touch/browser items in the
   [controls checklist](CONTROLS_CHECKLIST.md).

## Remaining engineering and acceptance work

- Add a persisted metric/imperial units setting in web setup, applied consistently
  to the device display and web dashboard. Keep stored measurements and service
  upload units independent of presentation; verify conversions and unit labels.
- Change the main-screen pressure label from "Baro" to "Barometer" and verify
  it fits the existing tile across themes.
- Compare CC1101 reception/field values with the RTL-SDR reference; measure
  debug-header voltage under simultaneous radio/display/Wi-Fi load. Live RF
  alone does not establish spare regulator capacity or antenna performance.
- Validate prolonged TLS/flash/RF load, transmitter silence and unchanged-data
  recovery, natural-midnight rain, clock/DST transitions, abrupt power loss,
  extended network outages and a multiday soak.
- Verify separate CWOP registration/routing/ingestion and uploaded field
  accuracy for each destination. A connected session or sent TCP packet alone
  does not establish remote acceptance.
- Test corrected hosted OTA beyond 90 seconds and a subsequent restart,
  interrupted transfer and deliberate failed-boot rollback before unattended
  installation. Automatic checking/installation default off and remain
  owner-controlled; publishing source does not enable them.
- Expand station integrations with real community captures, identity/field/
  unit/integrity review and appropriate OOK/FSK receiver profiles. Discovery
  confidence must fit each station's transmission cadence. See
  [decoder scope](../DECODER_SUPPORT.md) and [candidates](../STATION_SUPPORT.md).
- Add Windy, then consider PWSWeather. D710 serial and a local barometer
  are optional additions with unresolved hardware/pin requirements. SD remains
  optional. Calibrated lux-based cloud inference is deferred; current icons
  derive from NWS forecasts.

Repository: [sideout909/WX-Link](https://github.com/sideout909/WX-Link).
Original project code is GPL-3.0-only; dependency notices remain separate.
Keep credentials, captures, backups and release/build artifacts out of source.


## Local-model handoff — WXLink Connect

The owner requested a separate Documents/WXLink Connect workspace for community
capture firmware planning. [TODO_AUDIT.md](TODO_AUDIT.md) separates completed
features from remaining validation/roadmap work. Main v0.7.3 code, running
logger/device and release settings are unchanged by this handoff preparation.
