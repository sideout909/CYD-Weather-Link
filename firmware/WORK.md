# Current development state

## Published/installed prerelease: 0.9.0-rc.1

Stable/Dev selection and mandatory signed OTA are implemented locally, with
repository editing unlock and revised WX Link header/subtitle/GitHub link.
Build, host, browser and actual mbedTLS signature tests pass. Signed package is
under dist/v0.9.0-rc.1; public prerelease installed through signature-enforced OTA. Publisher key is outside
source/device; see SIGNED_OTA.md before any distribution. Owner confirmed
PWSWeather ingestion and plans cloudy-day lux, overnight and later reset checks.
Connect's public waveform suite passed read-only review (16 fixtures/40 packets);
all 20 decoder C sources match our pinned library. See CONNECT_WAVEFORM_REVIEW.md.

## Prior stable release: 0.8.2 — October 22, 2026

Published stable remains **0.8.2**; device now runs prerelease **0.9.0-rc.1**. The public GitHub image
installed through OTA, passed portal startup checks, confirmed at 33 seconds and
remained running beyond 100 seconds. The semantic-settings validation/save-lock
deadlock and resulting 0.8.1 rollback are fixed; normal OTA acceptance is complete.
Windy's verified-root TLS fix is retained and ingestion is owner-confirmed.
No credentials, DNS settings or weather history were changed by the repair.

The 0.8.0 feature batch is included: scoped factory reset, unit preferences,
Barometer label, PWSWeather/Windy, configurable hostname, updated touch targets,
layered sun, cleaner web tabs, fresh AWC pressure fallback and experimental local
lux clouds. Owner calibration remains enabled; other units default off/uncalibrated.
Metric/custom-hostname persistence and mDNS resolved on the bench; original
imperial/wxlink preferences were restored. PWSWeather ingestion is owner-confirmed; destructive reset,
cloud accuracy and extended acceptance checks remain open.

Current assignments and remaining gates: [TODO_AUDIT.md](TODO_AUDIT.md),
[FEATURE_QUEUE.md](FEATURE_QUEUE.md), [CONTROLS_CHECKLIST.md](CONTROLS_CHECKLIST.md).
Earlier records below retain their original version-specific evidence and may
mention restrictions or pending checks that were subsequently superseded.

## Historical local v0.7.6 work — included in 0.8.0-rc.1

Built locally; not flashed, pushed or released. USB has been handed back to the
owner for Connect testing; make no further device calls without coordination.

- Owner markup: left header x 0–103, y 0–43 opens combined Outdoor/status;
  x 104–319 across both header rows opens Settings. These regions are independent
  of theme and APRS/CWOP text width. First dim/off contact still only wakes.
- Outdoor now includes temperature/light/UV with ages plus Atlas health,
  last packet, missing message types, APRS/CWOP state and last report. Separate
  Link Status navigation is removed; the old USB detail index aliases Outdoor.
- On-device WEB SETTINGS explicitly says “Change settings in a web browser”
  and “Use your phone or computer”, above wxlink.local/IP and admin guidance.
  First-run WEB SETUP also names the browser workflow. Custom passwords stay hidden.
- Daytime icons now use a separate sun layer and condition/cloud overlay, like
  the existing moon composition. Home and forecast, display and webpage share
  this approach; unknown/stale forecast handling and phase-aware moons remain.
- SC05_X build, host suite (including full header-boundary coverage), and the
  browser suite passed; browser checks exercise sun/cloud layers across themes
  and retain moon-phase/night-overlay regression coverage.
- Revised local header overlay: captures/firmware-validation/touch-map-v076-preview/index.html.
  Uses old v0.7.5 capture backgrounds, explicitly labeled with proposed v0.7.6
  routing. Physical layout/touch checks await the next authorized installation.

## v0.7.5 — published release and owner OTA verification

The owner verified the v0.7.4 dashboard cleanup and reports exercising the
preceding firmware over the last 24 hours. v0.7.5 includes the renamed default
update source and migrates a saved old project source while preserving custom
forks. Build, host checks, package integrity and unauthenticated public download
verification passed. The owner installed it through OTA after saving the renamed
repository and confirmed the requested sustained/restart checks. Hosted
interruption/deliberate rollback and remaining hardware/service gates remain open.

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
  Normal hosted OTA to v0.7.5 is owner-confirmed; interruption and deliberate
  failed-boot recovery tests remain open.

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

The owner's authoritative implementation order and current checkpoints are in
[FEATURE_QUEUE.md](FEATURE_QUEUE.md). Factory reset, units and Barometer label
are implemented, tested and installed; specific physical acceptance remains open.

- Signed OTA/channel code and negative signature tests are local software complete.
  Validate on target, back up publisher key before distribution, and implement
  future dual-key rotation/recovery. Keep custom USB reflashing available;
  no Secure Boot, flash encryption or permanent eFuse changes. See SIGNED_OTA.md.
- Support explicitly selected multiple measurement sources (for example separate
  temperature, wind and pressure devices) feeding one station, with individual
  source identities/freshness. Deferred; configurable hostnames were separately
  restored and implemented in 0.8.0-rc.1.

- Verify the implemented factory-reset feature: authenticated web control, destructive
  confirmation, and physical recovery for a forgotten password. Define settings/
  history erasure clearly, regenerate setup credentials and return to setup.
  Scope erasure to project NVS/files; preserve bootloader and partition layout.

- Verify the implemented persisted metric/imperial setting in web setup, applied consistently
  to the device display and web dashboard. Keep stored measurements and service
  upload units independent of presentation; verify conversions and unit labels.
- Main-screen pressure is now "Barometer"; verify
  it fits the existing tile across themes.
- Compare CC1101 reception/field values with the RTL-SDR reference; measure
  debug-header voltage under simultaneous radio/display/Wi-Fi load. Live RF
  alone does not establish spare regulator capacity or antenna performance.
- Validate prolonged TLS/flash/RF load, transmitter silence and unchanged-data
  recovery, natural-midnight rain, clock/DST transitions, extended network
  outages and a multiday soak. Power-loss/history-retention stress checks are
  lower priority at the owner's request; they are deferred, not marked passed.
- Verify separate CWOP registration/routing/ingestion and uploaded field
  accuracy for each destination. A connected session or sent TCP packet alone
  does not establish remote acceptance.
- Test interrupted OTA transfer and deliberate failed-boot rollback before
  unattended installation. Normal v0.7.5 OTA, beyond-90-second operation and
  subsequent restart are owner-confirmed. Hosted 0.8.2 startup confirmation and
  beyond-100-second operation also passed. Automatic checking/installation default off and remain
  owner-controlled; publishing source does not enable them.
- Expand station integrations with real community captures, identity/field/
  unit/integrity review and appropriate OOK/FSK receiver profiles. Discovery
  confidence must fit each station's transmission cadence. See
  [decoder scope](../DECODER_SUPPORT.md) and [candidates](../STATION_SUPPORT.md).
- PWSWeather live ingestion is owner-confirmed; Windy v2 ingestion is owner-confirmed. D710 serial and a local barometer
  are optional additions with unresolved hardware/pin requirements. SD remains
  optional. Calibrated lux-based cloud inference is experimental in 0.8.0-rc.1; default icons
  derive from NWS forecasts.

Repository: [sideout909/WX-Link](https://github.com/sideout909/WX-Link).
Original project code is GPL-3.0-only; dependency notices remain separate.
Keep credentials, captures, backups and release/build artifacts out of source.


## Local-model handoff — WXLink Connect

The owner requested a separate Documents/WXLink Connect workspace for community
capture firmware planning. [TODO_AUDIT.md](TODO_AUDIT.md) separates completed
features from remaining validation/roadmap work. Main v0.7.3 code, running
logger/device and release settings are unchanged by this handoff preparation.

## Feature queue local implementation evidence

Factory reset now has an authenticated CSRF/origin-checked typed-confirmation
POST, browser cancel flow, and a deliberate on-device 10-second hold / release /
3-second Reset hold. It persists a pending marker in wxreset and executes scoped
wxlink settings/history/cache cleanup before workers at boot, with retry/fail-stop
on interruption. It does not format flash, touch Connect files or change eFuses.
New setup credentials are generated by the existing startup path. Logical deletion
is not a guarantee of forensic erasure. No real device reset was requested/tested.

Metric/imperial presentation is saved with default imperial migration; C/km/h/mm/
hPa apply to device readings, detail/trend/forecast values and browser/discovery.
Underlying raw fields, archive/CSV/API units and weather-service encodings stay
unchanged. Dedicated forecast wind descriptors convert; narrative remains NWS
wording. New host conversions and browser save/reload checks pass, including
missing/negative/zero values and raw API preservation. Barometer replaces Baro
on Home/web; font width fits the tile in host checks.

APRS software login and packet comment now use WX-Link in source; reporting tests
assert the packet identity. Local build and host/browser checks pass. All this
remains local v0.7.6; no flash, live service submission, push or release performed.
Owner is testing Connect on the USB-connected device; leave USB untouched.


## October 22 — 0.8.2 OTA validation repair

Owner reported startup settings rejection HTTP 409; protected device status
confirmed rollback to 0.8.1-rc.1. Moved the update save lock after all candidate
validation, retaining its position before credential selection and persistence.
Added a pending-boot-only no-op probe expecting HTTP 409; invalid probes still
expect HTTP 400. Build and host checks pass. Stable/Dev update selection is
recorded in TODO_AUDIT.md for later; no channel code implemented.

Published v0.8.2 after build/host checks and uploaded-asset byte comparison.
Verified public Latest manifest/image size and SHA-256, then installed via the
protected HTTP OTA controls on the owner device. Portal checks passed at eight
seconds, boot confirmation cleared at 33 seconds, and the device remained on
0.8.2 beyond 100 seconds. No USB flash, reset, credential change or channel code.


## 0.9.0-rc.1 lightning implementation and publication authorization

Owner selected option B, requested last-strike distance, five-second Barometer/
lightning rotation and thirty-minute unchanged expiry. Implemented bounded,
native-confirmed-only live tracking, conservative reset/gap/wrap baselines,
partial coverage, distance handling and theme-aware tile/detail rendering.
Removed Missing types from Outdoor/status and closed the remaining rows up.
Host/model/native adapter and packed-font tests pass; real storm validation open.

Owner subsequently authorized prerelease publication and device installation,
superseding the temporary no-device restriction while Connect was under test.
Device initially reports 0.8.2, healthy portal/radio and restored history.
A private 0.8.3-rc.1 bootstrap of this source will exercise the on-device verifier
before installing the signed public 0.9.0-rc.1 target. Bootstrap is not published.
Protected local publisher-key backup exists outside the repo; offline backup
remains recommended. Device backup and final bench result follow separately.


## October 22 — 0.9.0-rc.1 signed prerelease acceptance

Published v0.9.0-rc.1 as a prerelease, leaving stable Latest at v0.8.2. Public
assets matched the local package; manifest signature and image hash/size passed.
A private USB bootstrap with the signature verifier installed the public target
through real GitHub OTA. Device rejected unsigned Stable, verified the Dev
publisher signature, confirmed startup and remained running beyond 100 seconds.
Original update preferences were restored; a controlled restart retained
0.9.0-rc.1 with portal checks passed and checkpoint/history restored.
Free heap was 128,276 bytes and loop-stack free space 9,900 bytes after restart.
No factory reset, replay, synthetic strikes or owner credential changes occurred.
A protected full-flash backup was retained before installation. No claim is made
that the latest RAM-only weather state was checkpointed.

Real-storm lightning/distance checks, interrupted OTA, deliberate failed-boot
rollback and overnight dim/touch reliability remain open. Radio initialized but
initial post-restart diagnostics showed zero decoded messages, with captures
reaching the decoder and no capture/decoder/application queue drops. A runtime-only
capture-margin reduction from 9 to 3 dB recovered live Atlas reception: one accepted
packet and two duplicates, sensor status Receiving. Left at 3 dB for the overnight
run; reboot restores the unchanged 9 dB firmware default. This is a bench workaround,
not a permanent RF tuning change or an SDR comparison.
