# Repository Guidelines

## Project Structure & Module Organization

This repository contains hardware and firmware planning documents, display concepts/assets, a PC-side RTL-SDR reference-capture tool, and initial PanelLan ESP32-S3 firmware. The core firmware includes Atlas/CC1101 input, display/history, protected setup, persistence, NWS clients and initial exporters; live RF, network/service ingestion and soak validation remain open. Windy is follow-up work; D710 serial output, sound and barometer are optional additions.

- `DESIGN.md` defines the all-in-one weather appliance, data handling, interfaces, and acceptance criteria.
- `PARTS.md` proposes an ESP32-S3 hardware baseline and purchasing specifications.
- `HOME_SCREEN.md` specifies the main display; `SCREENS.md` defines swipe navigation, rolling 24-hour trends, and the proposed local NWS forecast page.
- `WEATHER_SHARING.md` records current sharing-network research, exporter priorities, and APRS/CWOP routing/registration validation requirements.
- `firmware/src/` contains the Arduino application; `firmware/include/` contains the weather model, packet/report adapters, service contracts and embedded setup page.
- `firmware/include/rolling_rain.h` tracks bounded rain-counter intervals for hour/24-hour coverage; checkpoint schema 3 persists them alongside minute history.
- `firmware/include/history_archive.h` and `firmware/src/archive_storage.cpp` implement the fixed seven-day/five-minute archive; `firmware/RECORDING.md` documents bounded storage and protected CSV/USB export. Replay must never enter the archive.
- `firmware/include/daily_rain.h` supplies local-day rain with an immediate midnight display reset, independent of when the next radio packet arrives.
- `tools/pack-weather-icons.py` packs existing original assets; `tools/verify_setup_page.py` optionally checks the embedded page in headless Brave/Playwright.
- `tools/pack-home-fonts.py` packs the v2 study's DejaVu glyphs into `firmware/include/home_fonts.h`; source font licensing lives in `assets/fonts/`.
- `assets/web/` contains local Leaflet 1.9.4 and the browser ZIP/map/elevation module; `tools/pack-setup-assets.py` creates deterministic gzip firmware assets. Optional `tools/verify_location_setup.py` mocks every map tile/provider request.
- `firmware/include/setup_credentials.h` separates the visible generated AP key from custom admin credentials; legacy migration must preserve private admin login.
- `firmware/include/sensor_health.h` distinguishes missing hardware, stale station cycles and silent transmitters; do not infer a freeze from stable weather values alone.
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

The GitHub repository is `sideout909/CYD-Weather-Link`; this checkout uses `main`. Keep private captures, backups, credentials and build caches ignored. Use concise, imperative subjects such as `Document CC1101 pin requirements`. Keep changes focused. Pull requests should explain the change, affected requirements, validation performed, and unresolved hardware assumptions; link relevant issues when available.

## Configuration & Architecture Constraints

Preserve the single-controller architecture. The owner removed SD packet logging as a requirement on October 20, 2026; SD is optional and must not be required for normal operation. Keep bounded history and controlled persistent-state writes; select and validate storage against the actual board. Verify GPIO allocation and D710 electrical requirements before wiring. Keep Wi-Fi credentials, upload keys, and APRS authentication details out of source, fixtures, logs, and exports.
