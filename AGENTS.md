# Repository Guidelines

## Project Structure & Module Organization

This repository contains hardware and firmware planning documents, display concepts/assets, a PC-side RTL-SDR reference-capture tool, and PanelLan ESP32-S3 development firmware. The core firmware includes Atlas/CC1101 input, display/history, protected setup, persistence, NWS clients and opt-in exporters. Live Atlas RF and owner-observed APRS.fi/WU/Weathercloud ingestion are confirmed; normal hosted OTA to installed stable 0.8.2 and Windy ingestion are confirmed. SDR comparison, separate CWOP ingestion, interrupted/failed-boot OTA and extended soak validation remain open. PWSWeather ingestion is owner-confirmed; D710 serial output and barometer are optional additions. Sound was removed from this hardware version on October 21; earlier audio plans are archived for a possible larger display.

- `DESIGN.md` defines the all-in-one weather appliance, data handling, interfaces, and acceptance criteria.
- `PARTS.md` proposes an ESP32-S3 hardware baseline and purchasing specifications.
- `HOME_SCREEN.md` specifies the main display; `SCREENS.md` defines swipe navigation, rolling 24-hour trends, and the implemented local NWS forecast page.
- `WEATHER_SHARING.md` records current sharing-network research, exporter priorities, and APRS/CWOP routing/registration validation requirements.
- `DECODER_SUPPORT.md` documents pinned-library field/unit/integrity review and implemented Atlas discovery with explicit owner confirmation; `tools/inventory-decoders.py` regenerates the complete and reviewed 433 MHz inventories in `reference/decoder-inventory/`. Library presence is not enabled firmware compatibility.
- `firmware/include/station_discovery.h` and `discovery_packet.h` implement bounded native-only Atlas discovery and Iris/Notos detection previews; `assets/web/discovery.js` provides the protected setup flow. `tools/verify_discovery_setup.py` validates the actual embedded page with synthetic candidates. Never send unconfirmed/discovery-only samples to history or exporters.
- `firmware/src/` contains the Arduino application; `firmware/include/` contains the weather model, packet/report adapters, service contracts and embedded setup page.
- `firmware/include/rolling_rain.h` tracks bounded rain-counter intervals for hour/24-hour coverage; checkpoint schema 3 persists them alongside minute history.
- `firmware/include/history_archive.h` and `firmware/src/archive_storage.cpp` implement the fixed seven-day/five-minute archive; `firmware/RECORDING.md` documents bounded storage and protected CSV/USB export. Replay must never enter the archive.
- `firmware/include/daily_rain.h` supplies local-day rain with an immediate midnight display reset, independent of when the next radio packet arrives.
- `assets/themes/palettes.json` defines seven shared display/web palettes, including Night with a lower backlight; `tools/pack-display-themes.py` generates portable `display_themes.h`, embedded CSS and `theme_icons.h` from original SVGs. Runtime builds need no image tools. Watch/warning overrides retain their meaning and Paper uses legible stale colors.
- `tools/pack-weather-icons.py` packs existing original assets; `tools/verify_setup_page.py` optionally checks the embedded page in headless Brave/Playwright.
- `tools/pack-home-fonts.py` packs the v2 study's DejaVu glyphs into `firmware/include/home_fonts.h`; source font licensing lives in `assets/fonts/`.
- `assets/web/` contains local Leaflet 1.9.4 and the browser ZIP/map/elevation and phone dashboard modules; `tools/pack-setup-assets.py` creates deterministic gzip firmware assets. Optional `tools/verify_location_setup.py` mocks every map tile/provider request.
- `firmware/src/services.cpp` advertises `wxlink.local` via mDNS with HTTP on port 80; device/browser show a numeric IP fallback. Responder startup retries without blocking core weather operation. A validated saved hostname supports multiple units per LAN; default wxlink and numeric IP fallback remain.
- `firmware/include/factory_reset.h` and `firmware/src/factory_reset.cpp` implement a durable project-only reset before workers start; do not execute a real reset against owner data during tests.
- `firmware/include/weather_units.h`, `local_hostname.h`, `pws_weather.h`, `windy_weather.h` and `lux_cloud.h` keep conversions, safe naming, uploader fields/scheduling and experimental solar/light modeling portable for host checks. See `firmware/RELEASE_0.8.0.md` for prerelease acceptance gates.
- `firmware/include/setup_credentials.h` separates the visible generated AP key from custom admin credentials; legacy migration must preserve private admin login.
- `firmware/include/web_history.h` bounds the protected browser minute-history pages; `firmware/WEB_DASHBOARD.md` documents the phone overview, cached forecast/alerts and setup-first behavior.
- `firmware/include/display_preferences.h` defines persisted clock/date choices, display inactivity policy, UV levels and compact status layout; touch wakes the display without pausing reception or network services.
- `firmware/include/lunar_phase.h` projects current provider lunar data to forecast-period midpoints for small moon icons; missing data stays unknown. Night tint preserves phase/crater geometry.
- `firmware/include/sensor_health.h` distinguishes missing hardware, stale station cycles and silent transmitters; do not infer a freeze from stable weather values alone.
- Published/installed prerelease 0.9.0-rc.1 implements Stable/Dev channels, mandatory publisher signatures and optional lightning on Home; signed OTA/startup confirmation passed. Stable Latest remains 0.8.2. See `firmware/SIGNED_OTA.md`; private publisher keys stay outside the repository and device. No Secure Boot/eFuse changes.
- `firmware/src/firmware_updates.cpp` handles opt-in GitHub OTA; `tools/package-firmware-release.py` creates ignored `dist/` artifacts without publishing. Keep image/manifest/rollback validation requirements in `firmware/UPDATES.md`.
- `firmware/WIRING.md` maps the owner's CC1101 photo to exact expansion pins and power constraints.
- `reference/` stores public CC1101 and PanelLan manufacturer PDFs with source links; module header numbering remains in the wiring guide.
- `firmware/boards/` contains the actual PanelLan hardware manifest; `firmware/tests/` holds host C++ model tests. `firmware/README.md` documents scope, commands, and the bench validation record.
- `tools/firmware_console.py` and its test provide acknowledged USB replay and framebuffer capture.
- `tools/monitor_rf.py` records opt-in read-only RF diagnostics/native packets through an exclusive USB port; `firmware/RADIO_DIAGNOSTICS.md` documents RSSI limits, overnight reference timing and tuning baselines.
- `tools/atlas_capture.py` continuously logs decoded reference-receiver packets; `tools/test_atlas_capture.py` tests archive integrity and receiver lifecycle behavior.
- `CAPTURE.md` documents the PC logger, service commands, hardware context, and log schema. Local data lives in ignored `captures/`.
- `tools/70-cyd-weather-link.rules` and `tools/setup-esp32-access.sh` configure serial access for the owner's specific PanelLan board; see `PARTS.md`.

Keep architectural requirements in `DESIGN.md` and component specifications in `PARTS.md`. Reconcile changes between them explicitly. Once firmware begins, document its actual source, test, and hardware-profile directories here.

## Build, Test, and Development Commands

PlatformIO/Arduino is configured for `panellan_sc05_x` in `firmware/platformio.ini`,
with pinned dependencies and a custom board manifest. Run from the repository root:

```bash
bash tools/setup-firmware.sh
bash tools/pio.sh run -d firmware
bash tools/test-firmware.sh
bash tools/pio.sh run -d firmware -t upload --upload-port /dev/ttyACM0
bash tools/firmware-console.sh status
```

Build tools live in a separate user-local virtual environment to avoid a
PlatformIO package-installer issue with spaces in the repository path. Use the
wrappers above. The USB console requires exclusive port access. Follow
`firmware/README.md` for replay, backup handling, and hardware validation.

The PC logger uses packaged `rtl-sdr`, `rtl_433`, and Python's standard library. Run its tests with `python -m unittest discover -s tools -p 'test_*.py' -v`. See `CAPTURE.md` for recording and service commands; these are not a firmware build workflow.

For documentation work, use `rg --files` to inspect tracked project content and review Markdown headings, tables, and links before submitting changes.

## Coding Style & Naming Conventions

Use descriptive Markdown headings, short paragraphs, and bulleted requirements. Put commands, filenames, protocol markers, and identifiers in backticks. Specify units, electrical levels, and exact board variants wherever they affect behavior.

Firmware uses C++17, two-space indentation, descriptive snake_case data fields, and camelCase functions. Keep the weather model independent of Arduino and display libraries; keep board wiring in the hardware profile/vendor configuration. No formatter or linter is configured yet. Distinguish proposed behavior from bench-verified compatibility.

## Testing Guidelines

Firmware model checks compile as a host C++17 executable with assertions; the replay adapter and PC logger use standard-library `unittest`. No firmware coverage target exists yet. Use the acceptance sequence in `DESIGN.md` to guide future validation. Prioritize packet fixtures for missing and stale fields, rain resets and wraparound, time validity, and serial encoding. Hardware validation should compare reception with the existing SDR, capture serial records, exercise network outages, and include a multiday soak test. Record the hardware profile, procedure, and observed results.

## Commit & Pull Request Guidelines

The GitHub repository is `sideout909/WX-Link`; this checkout uses `main`. Keep private captures, backups, credentials and build caches ignored. Use concise, imperative subjects such as `Document CC1101 pin requirements`. Keep changes focused. Pull requests should explain the change, affected requirements, validation performed, and unresolved hardware assumptions; link relevant issues when available.

## Configuration & Architecture Constraints

Preserve the single-controller architecture. The owner removed SD packet logging as a requirement on October 20, 2026; SD is optional and must not be required for normal operation. Keep bounded history and controlled persistent-state writes; select and validate storage against the actual board. Verify GPIO allocation and D710 electrical requirements before wiring. Keep Wi-Fi credentials, upload keys, and APRS authentication details out of source, fixtures, logs, and exports.
