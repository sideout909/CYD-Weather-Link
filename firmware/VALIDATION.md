# Core firmware validation

Current installed prerelease: **v0.9.0-rc.1**; published stable remains **v0.8.2**.
See the latest signed-OTA acceptance entry below. Historical stable hosted OTA
passed startup confirmation at 33 seconds and remained running beyond 100 seconds.
Windy ingestion is owner-confirmed. This is a bench
log: older version sections describe the state at the time, not current
limitations. The latest results are below; [WORK.md](WORK.md) tracks remaining
work and [CONTROLS_CHECKLIST.md](CONTROLS_CHECKLIST.md) tracks owner checks.

## Windy follow-up — 0.8.1-rc.1

Owner Windy account was enabled but no uploads were accepted. Diagnostic attempt
on hardware resolved stations.windy.com normally through the configured LAN DNS,
then failed with HTTP -1 / TLS -12288. PC chain inspection and the prior bundle
failure on the sun provider identified the cross-signed Google trust path.

The GTS Root R1 PEM was downloaded from Google's PKI, matched by SHA-256 to the
PC's trusted root store, and used directly for Windy without disabling signature,
expiry or hostname checks. Hardware then returned HTTP 200 Accepted for processing
and a nonzero last-success timestamp using fresh live Atlas observations. No
fabricated/replay upload was submitted; Pi-hole settings and credentials stayed
unchanged. Build and host checks pass. The owner confirmed the Windy station online, then approved stable 0.8.1.
No published 0.8.0 asset was altered. See WINDY_TLS_FIX.md for details.

## v0.8.0-rc.1 prerelease bench checks — October 22

Owner reauthorized USB and requested an owner-test prerelease, including pressure
and lux implementation. Before installation, weather/archive checkpoints saved
successfully and a private full-flash backup was stored under ignored captures.
No backup, credentials, raw captures or NVS images are release assets.

- SC05_X build, host suite and synthetic browser checks pass. Host coverage
  includes durable scoped reset failure/retry, conversion boundaries, hostname
  validation, PWS/Windy fields/freshness, Windy scheduling/timer wrap, candidate
  version ordering and solar/light stabilization/replay/low-sun guards.
- USB upload succeeded; device reports 0.8.0-rc.1, restored history/archive,
  working display/radio/Wi-Fi, real Atlas packets and APRS connection. Startup
  HTTP authentication/schema/CSRF checks passed. The updated startup test also
  rejects invalid new settings and verifies new secret fields remain masked.
- Direct NWS and device KRNP samples both lagged four hours. Fresh NWS AWC
  pressure was accepted on-device without relaxing the 90-minute age limit.
  Provider NWS AWC, station KRNP and Altimeter setting are explicit. See NWS_PRESSURE.md.
- Actual browser checks pass for secret masking, negative configuration requests,
  current fallback pressure, Overview-only Sun & Moon, top connection health and
  closed-by-default/tappable wiring. Local screenshots are ignored in captures.
- Temporary metric/hostname save/reboot succeeded. `wxlink-bench.local` resolved
  via avahi to the device IP. Metric display was captured. Restored original
  imperial/wxlink preferences; calibration and original exporter enable flags
  survived. New PWS/Windy uploaders remain off, with no test uploads submitted.
- Owner confirmed a cloud-free day. Median normalized calibration used 153 minutes
  of today's light and was saved/enabled. Startup correctly showed warmup while
  awaiting distinct fresh samples. Bench caught whole-second/anchored RF clock
  mismatch during warmup; using the same millisecond snapshot clock fixed it.
  Five distinct samples over 122 seconds then produced Likely clear; six over
  184 seconds retained it. Live web source became LUX*; owner requested centered
  LUX without an asterisk on the display only.
  See LUX_CLOUDS.md for experimental limitations and thresholds.

Final-image snapshots showed roughly 72–123 KiB free internal heap across
startup/TLS and steady operation, with about 10.7 KiB loop-stack high-water
headroom. These are snapshots, not worst-case soak measurements. Device-specific artifacts remain ignored under
captures/firmware-validation/v080-*.json and PNGs.

A real destructive reset was not executed against owner data. Physical touch,
cloudy-day/seasonal classifier accuracy, new service-account ingestion, interrupted
OTA and the reported overnight touch failure still require owner acceptance.
The owner subsequently reported successful operation and approved publication as
stable 0.8.0. Only the version identifier changed for the stable build; a fresh
SC05_X build and host suite passed. The installed device was left on rc.1 for an
owner OTA test; no stable OTA result is claimed yet.

## Local v0.7.6 build — header targets, Outdoor/status and layered sun

Owner supplied captures/firmware-validation/touch-map-v075/home-overlay owner markup.png.
Implemented header split at x=104 over y=0–43, consolidated Outdoor/status,
explicit browser-settings guidance and composed daytime sun/condition layers.
The existing screenshot credential guard remains unchanged. Legacy USB detail
index 5 now aliases Outdoor index 0; no separate Link Status screen remains.

SC05_X firmware build and host checks passed; header tests cover both sides of
the boundary over all 44 rows and ensure body/out-of-bounds points do not route
as headers. The browser suite passed day sun/cloud layering across themes,
night overlays, moon-phase changes, setup and existing dashboard checks.
The changed forecast test selects base celestial images separately from the
new overlay images so two-layer cards do not shift its night-period indices.

No v0.7.6 device installation, GitHub push or release was performed. Owner
requested USB be disconnected and left available for Connect testing; the last
console session completed, fuser showed no holder, and no further USB calls
were made. Physical combined-page/settings/sun appearance awaits owner review.

An installed-v0.7.5 pressure detail screenshot (before USB handoff) reported
“NWS pressure observation too old or invalid”; no accepted station/sample was
shown. This is a combined failure message, not proof NWS returned stale data.
Investigation remains a non-critical priority bug; no freshness rule was changed.

## Owner checks and touch-target review — October 22

Owner confirmed setup-network hostname access, all themes, normal Dim/Off sleep
and wake, and forecast moon appearance. Home-LAN hostname had already been
confirmed. This does not resolve the separate overnight touch failure or assert
that every detailed outage/contrast/recovery permutation was tested.

Owner will perform interrupted OTA testing; no result reported yet. Power-loss/
history-retention stress testing is lower priority because external services
retain successfully uploaded observations. This is prioritization, not a claim
that local persistence or unsent observations cannot be lost.

A source-derived touch map was generated from nine live v0.7.5 screenshots;
Settings and Alerts use labeled schematics to avoid setup credentials or a fake
alert capture. No replay was used, and the device was returned to Home. Local
artifact: captures/firmware-validation/touch-map-v075/index.html; static preview:
home-review.png. Rebuild with tools/generate-touch-map.py after refreshing PNGs.

Host/browser checks passed: exclusive full-screen pixel coverage, boundary checks
for Wi-Fi/measurement/alert regions, 11 screen views, APRS/CWOP geometry, gesture
layers and wake-only mode, with no JavaScript errors. These verify the generated
review artifact, not physical touch accuracy or the unresolved overnight failure.
The map is not empirical touch-frequency data; no firmware hit regions changed.
Review notes: Wi-Fi target is y 26–43 (18 px); Home y 184–187 is inactive; the
full y 0–25 header opens Settings. Await owner feedback before adjusting targets.

## Open observation — overnight dim/touch failure on v0.7.5

Owner reported the device remained in dim mode overnight and did not respond
to touch. It was already reset before the investigation. Post-reset USB status
reported v0.7.5, display initialized, brightness 160, idle 41 seconds, uptime
46 seconds and touch_events 3. Wi-Fi/radio/APRS were working; checkpoint/archive
restored with no reported archive failures. These are post-reset observations,
not a diagnosis of the failed state. A brief source review found no clear cause.
Owner requested stopping the deep investigation and will leave it running on
recurrence for read-only status/touch/wake checks. No firmware changes made.

## Owner-confirmed normal hosted OTA — v0.7.5

After publication, v0.7.4 initially reported “Release is incompatible or invalid”.
The public image URL named the new WX-Link repository while the device still
used the old saved source. Saving `sideout909/WX-Link` and restarting resolved
lookup/validation. The owner then tested OTA and confirmed all good after being
asked to verify beyond 90 seconds and perform a subsequent controlled restart.
This is owner-reported evidence; no separate post-update USB inspection was run.
No unexpected rollback was reported. Interrupted transfers and deliberate
failed-boot recovery were not exercised and remain separate acceptance items.

## Public release package — v0.7.5

The owner reports exercising the preceding firmware over the last 24 hours and
confirmed the installed v0.7.4 dashboard cleanup. v0.7.5 adds the WX-Link update
source/default migration; no USB installation was performed for this release.

- SC05_X build: passed; RAM 71,836 bytes, flash 1,698,557 bytes.
- `bash tools/test-firmware.sh`: passed model/adapter checks and nine console
  tests, including repository migration/custom-source preservation coverage.
- The setup browser checks passed after the repository-reference change.
- Release packager accepted the ESP32-S3 application image, board/layout,
  version and slot size. Manifest URL uses `sideout909/WX-Link` and v0.7.5.
- Application size 1,698,928 bytes; SHA-256
  `6e08a5842657c8f3d0e6ee4c33e47553efe5bb89a5b1c73bcaa89b2ee3ed595d`.
- The ZIP includes the image/manifest/checksums and GPL/third-party notices;
  CRC and image/manifest integrity checks passed. No device settings, NVS,
  capture, bootloader or partition payload is included.

This packages the verified source for release; hosted OTA interruption/rollback
and the remaining hardware/service validation gates are still open.

## Completed-setup dashboard cleanup — v0.7.4

- `tools/verify_setup_page.py`: passed synthetic browser checks, including the
  Get connected card remaining available for incomplete setup, hiding after
  completion, and retained Station & Wi-Fi top navigation.
- `bash tools/test-firmware.sh`: passed host checks and nine console tests.
- SC05_X build and USB upload: passed; reported RAM 71,836 bytes and flash
  1,698,221 bytes. Checkpoint/archive save succeeded immediately before upload.
- Initial device status: v0.7.4; display, Wi-Fi, radio and mDNS ready; checkpoint
  restored; archive ready with 293 records and zero archive write failures;
  portal authentication/CSRF/startup checks passed on attempt one.
- No replay was injected and no credentials were exported. This USB installation
  does not establish hosted OTA acceptance. No GitHub release assets published.

## Loopback startup check — local v0.7.3

The mDNS hostname resolved to the correct LAN IP after v0.7.2 installation;
unauthenticated HTTP through the name returned 401 and the owner independently
confirmed access. The device's separate startup probe nevertheless failed its
first AP-address connection on all three attempts (HTTP -1). This was a local
self-connection failure, not evidence of a broken external web service; the
precise AP/STA routing interaction is unconfirmed.

v0.7.3 changes only the probe target to 127.0.0.1, supported by the pinned SDK's
CONFIG_LWIP_NETIF_LOOPBACK. It still connects to the actual HTTP server and
performs all authentication, CSRF, schema and invalid-input checks. Retry limits,
30-second confirmation, 90-second rollback and watchdog remain intact. No mDNS
or Wi-Fi/internet success is made a boot prerequisite. Actual loopback success
and continued name access are verified after installation below.

Final v0.7.3 build: 1,698,173 application bytes (50.8%) and 71,836 static RAM
bytes (21.9%). Acknowledged checkpoint/archive flush preceded USB installation.
At 5.5 seconds uptime, protected portal checks Passed on attempt one; mDNS is
ready, the checkpoint restored 1,124 minute buckets and 241 archive records,
and live RF resumed. Avahi resolves wxlink.local to the board's LAN address;
curl through the hostname returns HTTP 401 without credentials as expected.
The owner independently confirmed the friendly address works. Setup view text
shows the hostname and numeric fallback; physical setup framebuffer capture
remains blocked to avoid exporting generated Wi-Fi keys. No replay or publishing.


## Friendly local address — local v0.7.2

Enabled the existing ESP32 mDNS responder as wxlink.local, with an HTTP/TCP
port-80 service and generic CYD Weather Link instance name. Initial failure
retries every 30 seconds; existing AP DNS and numeric addresses remain available.
The platform handles interface/address events. More settings and first-run
views show http://wxlink.local prominently, with numeric IP fallback. The
browser header also links both, and status/USB expose readiness without secrets.
No credential, CSRF, radio or upload gate was changed. Default name assumes
one unit per local network; blocked multicast/incompatible clients use the IP.

Host and browser checks passed, including both address links and name-startup
failure feedback. Protected startup checks verify the new status schema without
requiring successful mDNS resolution for boot. Build: 1,698,209 app bytes (50.8%
of OTA slot), 71,836 static RAM bytes (21.9%). Before installation, a targeted
lookup did not resolve wxlink.local on the owner's LAN. Physical setup view
rendering stays protected from framebuffer export because it can contain the
generated Wi-Fi access key.

## Night theme and forecast lunar cycle — local v0.7.1

Added seventh palette Night (low light): black/near-black surfaces, subdued
amber text/artwork, softened olive NWS all-clear/connected status and an
amber-tinted cratered moon. Text contrast remains checked at RGB565 precision.
Night backlight is 24/255 normally, 2/255 after configured idle Dim, and zero
for Off. Waking returns to 24 rather than 160; other themes retain 160/16.
The backlight stays off after display initialization until saved settings and
the first themed frame are ready. Watch/warning color semantics are unchanged.

Forecast moons now use provider phase/illumination anchored at noon of its
local reference date, advanced to each period midpoint with a mean 29.530588-day
synodic cycle. This is a small-icon estimate, not a precise ephemeris. The mean
period is documented by [NASA](https://eclipse.gsfc.nasa.gov/phase/phasecat.html).
Missing current-day lunar data shows an unknown moon. The browser applies
late-arriving phase data immediately; phase/craters/clouds remain compatible
with Night tint and hemisphere orientation. No extra provider calls are made.

Host checks passed for all weather models, seven-palette contrast, reduced
text/green/moon luminance, Night normal/dim/off/wake policy and lunar new/full/
quarter transitions, invalid data and forecast horizon. Browser checks passed
for all themes, softer NWS all-clear green, phase arrival updates, different
future-night phases and waxing/waning changes. Build passed: 1,670,997 app
bytes (50.0% of OTA slot), 69,908 static RAM bytes (21.3%). Actual dark-room
brightness and physical wake/restart behavior remain owner checks.

USB installation followed an acknowledged checkpoint/archive flush. The board
reports v0.7.1, passing protected portal checks, restored checkpoint, 1,113
minute buckets and 239 archive records. Wi-Fi and live RF resumed; current
lunar data reports Waxing Gibbous / 77.6%. Actual Forecast capture
`display-v071-forecast-live.png` shows a gibbous moon behind Tonight's fog art,
replacing the former crescent. Home was restored afterward. Existing saved
palette was preserved; physical Night brightness/selection remains an owner
check. Local Markdown links and whitespace checks pass.

## Shared palettes and sound removal — local v0.7.0

Added persisted display_theme for original Midnight and the five reviewed
Matrix/Amber/Ice Blue/Violet/Paper concepts. Shared JSON generates RGB565
palette constants, browser CSS and original SVG-derived weather icon variants.
The browser previews selections locally; Save and restart applies the saved
palette to both surfaces. Existing configs default to Midnight. Invalid theme
IDs/types are rejected. Watch/warning overrides remain separate, and Paper
uses a darker legible stale orange. Sound is removed from display, browser
status and status JSON; RF/WiFi/APRS shift right with the matching Wi-Fi target.

Host checks passed, including six-palette lookup/fallback, text contrast on
background/cards, stale/accent contrast and the right-aligned status geometry.
The protected browser suite verifies every palette/asset URL, preview without
reading/field loss, saved theme submission/reload, three status indicators and
Paper warning-card contrast. Setup/discovery/ZIP-map browser suites passed.
Pinned build: 1,650,017 application bytes (49.4% of the OTA slot), 69,908 static
RAM bytes (21.3%). Private Matrix/Paper browser captures were generated; Paper
form layout was visually reviewed. These are browser/source checks; owner
selection and physical color matching remain controls-checklist items.

USB installation was preceded by an acknowledged checkpoint/archive flush.
The installed board reports v0.7.0, migrated Midnight theme, connected Wi-Fi,
passing protected portal checks, restored checkpoint, 1,107 history buckets and
237 archive records. Radio is ready with zero reported drops and live packets
resumed in Receiving state. Actual framebuffer display-v070-home.png confirms
speaker removal and RF/WiFi/APRS alignment at the right edge. No replay was used.

Earlier sound design is archived in reference/FUTURE_AUDIO.md rather than
retained as an optional requirement on this board. GPIO and storage layout,
weather processing and exporter eligibility are unchanged. No GitHub publishing.

## Dashboard refresh connection status — local v0.6.3

Owner reported a transient Wi-Fi/NWS warning on refresh despite a connected
station. Cached alert responses could render before initial status; undefined
Wi-Fi was treated as false. The deferred dashboard could also miss an earlier
inline status response, waiting for the next five-second poll.

The page now retains early status and distinguishes unknown connection state
from confirmed disconnection. Pending status displays Checking device connection
without an artificial amber warning. Known cached warning events remain
prominent. Real failed requests/disconnections still mark data unverified.
Browser regressions hold first status until cached NWS arrives, then verify
real disconnection; a second page holds the deferred script until connected
status arrives and verifies immediate recovery without another status poll.
The complete setup/dashboard browser suite passed. Build passed with 1,604,033
application bytes and 69,892 static RAM bytes. No weather, RF or exporter logic
changed. Changes remain local. Before USB installation, checkpoint save and
archive flush were acknowledged. The installed board reports v0.6.3, connected
Wi-Fi, successful portal checks, restored checkpoint, 1,095 history buckets,
234 archive records and ready radio with zero reported drops. Actual phone
refresh confirmation remains an owner check.

## Receiving and night conditions — local v0.6.2

Renamed the displayed Collecting state to Receiving. The internal partial-cycle
state and all health/upload/archive rules are unchanged. The night's actual
forecast included patchy fog, so its previous neutral fog icon did not include
a moon. Recognized night fog/precipitation/wind conditions now retain a moon
behind their condition art on the panel and browser. Cloudy night overlays
remain; unknown conditions are not presented as clear nights. Forecast moons
are illustrative, while Home retains current phase when available.

Host checks and protected setup/browser regressions passed, including the
Receiving label and night fog layering on Home-equivalent and forecast cards.
Pinned build: 1,603,857 application bytes (48.0%), 69,892 static RAM bytes
(21.3%). No credentials, receiver tuning or exporter policy were changed.
Checkpoint save and archive flush were acknowledged before USB installation.
The installed board reports v0.6.2, restored checkpoint, 1,092 history buckets,
234 archive records and passing portal checks. Live packets resumed with the
new Receiving state and zero reported drops. Actual Forecast framebuffer
capture display-v062-forecast-night.png shows Tonight's patchy-fog moon above
the fog/cloud artwork; Home was restored afterward. Local links/headings across
all 25 tracked Markdown documents and git diff whitespace checks passed.

## Packet-age header — local v0.6.1

Added whole seconds since the last decoded bound-station packet beside the
source state. This includes repeats/flagged bound packets as reception evidence
without refreshing absent/invalid measurements. Other stations and replay do
not update this live counter. The status line uses available space before the
RF indicator and a smaller font for long state/age text.

Pinned build passed: 1,603,533 application bytes, 69,892 static RAM bytes.
Checkpoint save and archive flush were acknowledged before USB installation.
The board restored 1,088 history buckets and 233 archive records; portal checks
passed and live bound-station packet reception resumed with no reported drops.
A private framebuffer capture, display-v061-packet-age.png, shows
`Atlas · Collecting · 9s` without overlapping RF/WiFi/APRS/sound. This is a small
display-only change; observation freshness, archive and uploader gates are
unchanged. No replay was injected and no GitHub release/source was published.

## Display preferences and condition graphics — local v0.6.0

Implemented configurable date format, 12/24-hour clock, keep-on/dim/off display
inactivity, and a 60-second return from Trends/Forecast to Home. Display power
changes only the backlight; reception, history and networking continue. A wake
gesture is consumed before normal controls resume. Added the compact header,
post-setup settings guidance, original forecast icons/cloud overlays, NWS label
above the Home icon, and station UV value with the full level name (Moderate).
Custom admin credentials remain hidden on the display.

The complete host suite passed, including display preference/idle/UV/layout
checks and existing weather/archive/upload checks. Headless setup, station
discovery and ZIP/map/elevation browser checks passed. The pinned build uses
1,603,385 application bytes (48.0% of its OTA slot) and 69,892 static RAM bytes
(21.3%). These source changes remain local; no GitHub release was published.

Before USB installation, checkpoint save and archive flush were acknowledged.
After installation the board restored its checkpoint and 222 archive records,
and the protected HTTP startup self-test passed on its first attempt. The radio
subsequently reported Current with no dropped packets. Private framebuffer
captures display-v060-home.png and display-v060-forecast.png confirm the compact
header, source label and forecast artwork on the actual display. Initial Home
capture was during collection; it does not validate live daytime UV artwork.

A 65-second read-only RF observation received 21 packet records; after the
Forecast idle interval, status reported Home (page 0), Current reception,
1,046 history buckets and zero capture, decoder or application queue drops.
Default keep-on brightness remained 160. The private diagnostic file is
rf-followup/display-v060-idle.jsonl.

The owner subsequently reported the v0.6.0 display/control changes passed
their hands-on checks, including the priority items in CONTROLS_CHECKLIST.md.
This is owner-reported acceptance rather than instrumented timing/voltage data.
Weathercloud ingestion is owner-confirmed, rather than a
new automated provider-side check. Corrected hosted OTA acceptance remains open.

## Earlier bench records

## Setup touch target — v0.5.3, October 21, 2026

The owner reported hitting station status while trying to open Wi-Fi setup.
The padded Wi-Fi region (x=184–243, full header height y=0–43), plus the
existing title target, now opens setup. Other status indicators still open
Link status. The setup close header also uses 44 pixels. Removed the sun-data
attribution from the display's setup screen; browser attribution remains.
Build passes: 1,589,833 application bytes, 69,844 static RAM bytes. The request
recovery tests from v0.5.2 also pass; physical touch confirmation remains for
the owner. No radio/profile or credential changes are involved.

## Discovery request recovery — v0.5.2, October 21, 2026

Owner reported an unavailable error about one second into the first attempt;
a second attempt discovered the station. The exact failing request was not
captured. Source review found that an initial status failure after an accepted
Start could leave the page without ongoing polling, and an error from a lost
Start reply could overwrite a recovered active state.

The browser now resumes status checks after uncertain Start delivery, retains
polling after the first status failure, and serializes status requests. It
never automatically repeats the Start mutation or resets the five-minute
scan timer. Tests deliberately inject HTTP 503 for the first status request,
then an accepted Start with a dropped response plus a failed status read.
Both recover without another Start; the complete setup/dashboard browser suite
also passes. This establishes the recovery fix, not the exact cause of the
owner's original message. Build: 1,589,997 application bytes, 69,844 static RAM.

## Automatic station discovery — v0.5.1, October 21, 2026

- Added passive 433.92 MHz OOK Atlas discovery, with Iris/Notos detection
  previews. Three independent sightings plus fresh complete Atlas fields are
  required before selection. Settings save rechecks the scan session/token.
- Existing saved binding migrates as confirmed. New devices remain unbound;
  only trusted native RF callbacks enter discovery. Replay/foreign candidates
  do not enter operational archives or uploads. Existing bound reception keeps
  running during a scan. Broader profiles and FSK remain unimplemented.
- Host checks passed for burst deduplication, unchanged valid weather,
  identity/channel separation, per-field expiration, cancellation/session
  invalidation, five-minute timeout, capacity bounds, timer wrap and parser
  mappings. All previous model/reporting checks, nine console regressions and
  host replay of 24,075 private Atlas reference packets passed.
- Headless browser checks used the actual embedded HTML and local scripts:
  setup-first onboarding, hidden manual ID fields, authenticated/CSRF requests,
  eligible/unsupported candidates, staged selection, preserved unsaved Wi-Fi,
  cancellation, timeout, mobile width and no JavaScript errors. Existing full
  setup/dashboard and mocked ZIP/map/elevation checks passed.
- Build: 1,589,725 application bytes (47.6% of OTA slot), 69,844 static RAM
  bytes (21.3%). USB installed on the owner's PanelLan board after an
  acknowledged checkpoint save/archive flush; no filesystem erase.
- Boot restored 701 populated minute buckets and 147 archive intervals;
  protected HTTP self-test Passed on attempt 1, including discovery schema and
  CSRF rejection. External unauthenticated GET/POST discovery endpoints and JS
  asset all returned HTTP 401.
- A subsequent 60-second native RF monitor captured 18 packet copies: six
  each of Atlas types 37, 38 and 39. Capture/decoder/application queue drops
  remained zero; final required field/type ages were below 25 seconds and
  health Current. Running those exact packets through the host discovery
  model yielded six independent sightings and an eligible Atlas candidate.
- Final board status: v0.5.1, 702 populated history buckets, 147 archive
  intervals, no archive failures, Current, Wi-Fi and APRS connected. Free heap
  138,436 bytes, free PSRAM 1,725,623 bytes, loop stack minimum free 11,052 bytes.
  RF baseline remains 433.92 MHz / 9 dB adaptive capture margin.
- Owner phone scan/select/save and touch checklist remain open; synthetic
  browser tests and host-fed native RF are not an actual phone confirmation.
  See [priority checks](CONTROLS_CHECKLIST.md#discovery-checks-from-v051).
  Corrected hosted OTA validation and additional-station hardware tests remain
  open. This update was USB-installed, not published as a new OTA release.

The earlier records below remain as historical validation for prior versions.

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

## Acceptance gates recorded before live CC1101 connection

Historical checklist: initial reception and APRS.fi/WU/Weathercloud ingestion
were subsequently observed. Use [WORK.md](WORK.md) for the current open gates.

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

## Moon appearance update — v0.4.1, October 21

Built the `panellan_sc05_x` image with a shared C++ moon renderer. Reviewed
an enlarged host rendering of the actual renderer for new, waxing crescent,
first quarter, gibbous, full and waning crescent phases. Gray crater markings
remain clipped to the existing illuminated-phase mask; the shadow and rim are
neutral gray. Clear/partly cloudy nights share the renderer. Existing icon
size/position, phase direction, missing-phase indicator and NWS freshness gates
are preserved. Preview: private `captures/firmware-validation/moon-v041-preview.png`.
The host rendering compiles with `-Wall -Wextra -Werror`.

The owner disconnected USB after v0.4.0 and requested a manual OTA test.
The LAN portal responds with HTTP 401 as expected without admin credentials.
Physical appearance and actual OTA installation/startup confirmation are pending;
the build alone does not establish either. No USB upload was attempted.

## Update defaults and status feedback — v0.4.2, October 21

The owner reported a persistent "Check queued" message. Found the browser's
separate action acknowledgement was never replaced after completion; this
does not establish that the updater worker itself was stuck. Replaced it with
the current `/api/update` message and progress, including terminal failures.
The owner was asked for the actual Current/version status line after refresh.
The unauthenticated LAN portal remains reachable with expected HTTP 401.

New settings use the project repository; trimmed empty values migrate to that
default while custom repositories and opt-in check/install flags are preserved.
The pinned firmware build passed (1,536,321 application bytes). Browser checks
passed, including queued → lookup failure → retry → available → download
progress → healthy startup, default repository population, and unchanged
settings/save/map-page behavior. These are mocked browser states, not an
on-device OTA result. No USB upload or unattended installation was performed.

The owner's refreshed v0.4.0 status then reported "Set a public GitHub
owner/repository first" after briefly showing checking/0%. This establishes
the request was processed and the running configuration had no repository;
it was not a stuck worker or a demonstrated GitHub/TLS failure. Instructed the
owner to save the repository and allow the settings restart before retrying.
The old action acknowledgement had concealed this actionable terminal result.
The separate location browser checks also passed. The published v0.4.2 latest
manifest and downloaded firmware size/SHA-256 were verified anonymously.


## First manual OTA installation — owner confirmation, October 21

After saving the repository and retrying, the owner reported the OTA update
"worked flawlessly" and that the moon "looks fantastic". The offered latest
release was v0.4.2, upgraded from the previously displayed v0.4.0, with USB
disconnected. Record the manual hosted update and physical moon appearance as
owner-verified. No authenticated post-update API capture or explicit startup-
confirmation message was independently observed. This does not validate
interrupted transfers, deliberate failed-boot rollback, automatic installation,
or archive/settings preservation through an exported before/after comparison.


## OTA rollback report and correction — v0.4.3, October 21

**Supersedes the earlier first-OTA success classification.** The owner subsequently
reported the display reverted to v0.4.0. Transfer/initial execution and the moon
appearance were observed; final boot confirmation was not independently seen.
The exact reset/failing self-test remains unconfirmed. Requested elapsed time,
power continuity and the old firmware's portal_tested/portal_passed/uptime_s.
Held v0.4.0 as latest and marked v0.4.1/v0.4.2 prereleases to stop offering them
as normal updates. Do not disable rollback to make this test appear successful.

Found an actual readiness race: the network worker ran a one-shot HTTP self-test
1.8 seconds after task creation, while main setup could still be scanning the
archive/starting the receiver and not servicing WebServer. A timeout was sticky
and could deny confirmation. This is a source finding, not a captured device
failure. v0.4.3 waits for main-loop HTTP service, retries failed probes at most
three times with 1.5-second spacing, and records attempt count/stage/HTTP code.
Fresh clients isolate each probe's credentials/headers. No authentication,
secret masking, archive schema or CSRF test was removed.

Also block valid settings-save restarts during pending boot verification, so
an early Save cannot itself cause rollback. CSRF and invalid station checks
precede the busy/pending guard, preserving their expected rejection responses
for self-tests. Updater status explains a waiting check or confirmation-write
failure. Confirmation remains after 30 seconds with display + portal healthy;
unconfirmed firmware rolls back after 90 seconds. Radio reception is not a gate.

Host startup tests cover delayed HTTP readiness, retry spacing/exhaustion,
success stopping retries, timer wrap, and both original boot deadlines. The
complete host suite and browser regressions passed; browser tests include a
pending-boot failure reason and disabled Save/Check/Install until confirmation.
The pinned build passed: 1,535,533 application bytes, 68,324 static RAM bytes.
These are source/host results; corrected OTA installation remains pending.


## Read-only RF monitoring candidate — October 21

Added USB rf_status/rf_monitor commands and tools/monitor_rf.py. Diagnostic
snapshots use the receiver library's cached status (no extra SPI reads/tuning).
Packet records are emitted only while explicitly enabled with a 15-second host-
heartbeat lease; replay never emits production RF records. The host holds one
exclusive port, records JSONL, and disables streaming on exit. Existing status,
weather history, radio settings and upload gates are preserved.

The current v0.4.3 build uses 1,538,601 application bytes and 68,332 static RAM
bytes. All 18 Python tests pass, including malformed/unrelated output filtering,
read-only command scope and stream cleanup on device errors. Firmware host
startup/boot-policy tests and browser startup controls passed earlier in this
candidate. Added RF code compiled in the actual pinned build.

The owner reconnected with an uncertain longer USB-C cable; 303a:1001 enumerated
and the existing USB console worked. Before upload, v0.4.0 reported portal_passed
true, no heard target packets in this boot, archive_ready true, 11 records and
zero archive write failures. Exported that CSV before USB installation. The new
candidate reports portal_passed true / attempt 1 / Passed by 5.1 seconds uptime,
radio ready, 11 records preserved and zero archive errors. A ten-minute RF
baseline is being recorded privately in rf-20261021/baseline.jsonl. No tuning
change or SDR service restart was performed. Corrected hosted OTA remains open.


## Antenna trial, final header and NWS pressure — v0.4.5, October 21

Recorded 475.9 seconds of mixed antenna observation at the original 9 dB margin.
The owner changed back to clipped coax during this capture (exact change time
unknown). Later it still crossed Data stale/No signal, with zero capture,
decoder-queue and application-queue drops. At the owner's request implemented
an explicit RAM-only margin command (3–15 dB), and tried 6 dB. Its effective
threshold is logged separately because the library recalculates on its next
average window. One first acknowledgement was lost while the setting applied;
idempotent timeout retry and a regression cover that transport case.

After the owner confirmed the small antenna, about 201 seconds of observation
contained one distinct temperature report / three copies and a 37.2-second
capture. The background rose to about -63 dBm and threshold to -57 dBm; all
weather types aged out despite the lower margin. No queue drops were observed.
The owner chose coax overnight and a VNA check in the morning. This does not
prove antenna resonance or independently identify the interference source.
Private RF JSONL, owner observations and plots remain in rf-20261021/.

v0.4.4 changed the actual header order to RF, WiFi, APRS/CWOP, trailing sound
icon and removed the link outline. v0.4.5 removes the NWS-forecast suffix from
local-temperature/lux text, renames BARO and adds a clearly labeled nearby NWS
pressure observation. The Atlas has no local barometer; local priority remains
future hardware work. Pressure is display-only and excluded from uploads and
archives. Prefer sea-level pressure; otherwise report the API barometric field
with its type in details. Validate units/ranges, reject missing/nonfinite,
future/90-minute-old observations and known bad QC flags, poll about ten minutes,
respect retry delays, and try another nearby station after unavailable data.

Pinned build passed: 1,544,021 application bytes, 68,492 static RAM bytes.
Host pressure tests cover units/bounds/NaN/zero/future/expiration and all earlier
firmware checks passed. Python regressions passed, including explicit margin
scope and lost acknowledgement retry. USB upload restored checkpoint schema 3
and all 15 archive records; portal self-test Passed on attempt 1. Actual pressure
page showed NWS / KRNP, reported barometric pressure 30.33 inHg, observed 27 min
ago. Main screenshot verifies the row order, no APRS outline, lux-only line,
BARO number/units and NWS directly beneath it. Returned to Home afterward.
Corrected hosted OTA acceptance remains open; no automatic update was enabled.

### October 22 — 0.8.2 hosted OTA acceptance

Owner device SC05_X upgraded from rolled-back 0.8.1-rc.1 via public GitHub OTA.
Public manifest/image hash and size verified. Startup portal validation and
pending-boot save-lock probes passed; confirmation cleared at 33 seconds and
0.8.2 remained running beyond 100 seconds. No rollback observed. This does not
claim interrupted-transfer or deliberate failed-boot validation.


### Local 0.9.0-rc.1 software validation — October 22

SC05_X build and host suite pass. Browser suite checks channel save, repository
read-only/unlock/relock, WX Link title and project link, alongside existing flows.
The actual firmware ota_signature.h verifier compiled against mbedTLS 2.28.7
accepts valid Stable/Dev manifests and rejects metadata changes, wrong keys/repo,
missing/invalid/truncated signatures and unsupported schema/algorithm. Ephemeral
keys only in regression tests; signed local packaging separately used the local
publisher key. Public trust is in source; private key is outside this checkout.
No device access, upload, publication, factory reset or account change performed.
Installed stable remains 0.8.2. See SIGNED_OTA.md for hardware/release gates.
Owner separately confirmed PWSWeather ingestion; this is not a new host-side
field comparison. Cloudy-day lux, overnight and reset checks are planned.
Read-only Connect replay passed 16 public fixtures/40 callbacks; no new production
station enabled. See CONNECT_WAVEFORM_REVIEW.md.


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
