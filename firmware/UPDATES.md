# GitHub firmware updates

The development firmware implements opt-in updates from a **public GitHub
repository**, with manual Check/Install controls and optional daily automatic
installation. Both automatic settings default off. The project repository is `sideout909/CYD-Weather-Link`; enter it in Updates.
No repository is hardcoded in the device and no GitHub token is needed. A real hosted release/download/rollback test
is still required before calling unattended updates bench validated.

## Owner setup

1. On the setup page, open Updates and enter `owner/repository` for the source
   you trust. Save and restart; this does not enable checks or installations.
2. Check now retrieves the latest compatible release manifest. A normal stable
   release needs the two named assets below; a draft/prerelease is not the
   normal GitHub `releases/latest` target.
3. Install update and restart appears only for a newer compatible version.
   Automatic checking is a separate opt-in checkbox; automatic installation
   requires both checkboxes. Automatic installation waits during NWS watches/
   warnings or missing/stale/unverified NWS status when a location is enabled.

The repository is the update authority. Verified HTTPS plus the manifest's
SHA-256 protect download integrity; there is no separate publisher signature
in this version. This does not enable secure boot or program security eFuses.
Use a controlled release source and validate a release before unattended use.

## Release preparation

The board-specific image is an application-only binary, never the full-flash
hardware backup. NVS credentials and local captures are not release assets.

```bash
bash tools/pio.sh run -d firmware
bash tools/test-firmware.sh
python tools/package-firmware-release.py --repo OWNER/REPOSITORY
```

The version lives in `include/firmware_version.h`. Bump its three-number
version before building a new release. Packaging rejects the wrong image
format/chip, oversized images and a stale image without the current version.
It creates ignored `dist/` assets:

- `weather-link-panellan_sc05_x.bin` — application image for the owned profile.
- `weather-link-panellan_sc05_x.json` — schema/version, board, partition-layout,
  flash size, image size, SHA-256 and exact repository release-download URL.
- `LICENSE-DejaVu.txt`, `LICENSE-Leaflet.txt`, `THIRD-PARTY-NOTICES.md` and
  `third-party-licenses/` — retained dependency/asset notices. A selected project
  `LICENSE` is copied when present. Bundle notices with distributed binaries.

Upload these files to a normal GitHub Release tagged `vVERSION`. The script
**does not create/publish a release**. [GitHub's documented latest-asset link](https://docs.github.com/en/repositories/releasing-projects-on-github/linking-to-releases)
is used to locate the manifest. Private repositories, tokens, arbitrary OTA
hosts and cross-board updates are unsupported.

## Installation and recovery

The updater uses a separate background task so an image transfer does not
occupy the NWS/observation-uploader worker. Main-loop receiver handling and
touch/display continue. It enforces HTTPS redirects to GitHub's release hosts,
board/flash/partition compatibility, newer version, image/chip header, size
and SHA-256. It writes the inactive 3.3 MB application slot and selects it
only after image validation. Download/hash failures retain the previous image.

Before a controlled restart, the main task checkpoints valid live history/
daily rain. NVS, the partition table, bootloader and LittleFS are not replaced
by the application update. A replay never becomes a production checkpoint.

The pinned ESP32-S3 SDK enables OTA rollback. Arduino's early automatic
confirmation is deferred: the new app confirms after at least 30 seconds
with display initialization and the actual local portal self-test passed.
Absent radio/home internet alone does not fail that check. A failed check
triggers rollback after 90 seconds; a crash/reset before confirmation also
lets the bootloader revert. The main task has a 30-second watchdog. These
paths follow [Espressif's OTA rollback mechanism](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/ota.html),
but actual download/first-boot/failure recovery still need an on-board OTA test.
USB recovery remains available through the documented hardware profile.

## Before unattended use

Use a separate test release/repository and a higher version to exercise a
complete installation. Verify settings, history/rain, display/touch, portal,
radio, NWS and sharing after restart. Exercise interruption, wrong hash,
wrong board/layout, older version, and a failed first boot/rollback. Measure
heap and receiver behavior while OTA and NWS TLS requests overlap. Keep
automatic installation off until these acceptance checks pass.
