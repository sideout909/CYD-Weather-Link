# WX-Link 0.9.0-rc.3

Development prerelease for field testing on the PanelLan SC05_X. This release
contains the same firmware changes as rc.2 with a new version so devices
already running rc.2 can discover it through the Dev update channel.

- Corrects forecast °F/°C symbols in the display.
- Keeps the CC1101 capture threshold at 9 dB.
- Adds reset reason and minimum free heap diagnostics to USB status.
- Uses WX Link naming in the device setup page.

The spontaneous reboot cause remains unresolved. Interrupted OTA, deliberate
failed-boot rollback and extended soak validation remain open. This prerelease
is not production validated; Stable Latest remains 0.8.2.
