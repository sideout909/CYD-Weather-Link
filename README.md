# CYD Weather Link

A single-controller AcuRite Atlas weather appliance for the PanelLan
**ZX2D80CE02S-V10 / SC05_X** ESP32-S3 touchscreen and a 433 MHz CC1101 receiver.
Amateur-radio weather sharing is a primary use: direct APRS-IS/CWOP is separate
from optional D710 serial output. No Pi/gateway or SD card is required.

Development firmware includes a v2 Home display, swipeable 24-hour trends and
NWS forecasts/alerts, local-midnight rain, five-minute received wind peaks,
field-age/connection details, callsign/status header, protected browser setup,
embedded ZIP/map/elevation selection, persistence and opt-in sharing. Weather
Underground and Weathercloud adapters are implemented; remote ingestion,
live CC1101 reception and a full soak remain validation gates. Windy is later
work; D710, pressure and sound are optional additions.

## Start here

- [Firmware setup, build, upload and controls](firmware/README.md)
- [Exact CC1101 pinout: debug 3.3 V/GND and expansion signals](firmware/WIRING.md)
- [Owner touch/browser controls checklist](firmware/CONTROLS_CHECKLIST.md)
- [Current bench evidence and remaining checks](firmware/VALIDATION.md)
- [Embedded map sources, privacy and offline fallback](assets/web/README.md)
- [Opt-in GitHub updates and release packaging](firmware/UPDATES.md)
- [Manufacturer PDFs](reference/README.md)

The firmware targets this exact board revision and pin map. Check the wiring
guide's supply assumptions before attaching a radio. UI replay and decoded
JSON tests do not prove live RF compatibility. Never present a replay as live
weather or upload it to a sharing network.

Configuration secrets live on the device, not in source. A custom admin
password is hidden; the visible setup Wi-Fi key is separate. Keep `captures/`
and full-flash backups private; they are ignored. Build outputs live in ignored
`firmware/.pio/`, and local release packaging writes ignored `dist/`.

Architecture, component choices and interface studies are documented in
[DESIGN.md](DESIGN.md), [PARTS.md](PARTS.md), [HOME_SCREEN.md](HOME_SCREEN.md),
[SCREENS.md](SCREENS.md) and [WEATHER_SHARING.md](WEATHER_SHARING.md).


## Licensing

Copyright (C) 2026 sideout909. Original project code is licensed under the
**GNU General Public License version 3 only (GPL-3.0-only)**; see [LICENSE](LICENSE).
You may use, modify and redistribute it, including commercially, under those
terms. This software is provided without warranty.

Third-party code, fonts, map assets and manufacturer references retain their
own notices and licenses; see [third-party notices](third_party/README.md).
