# GitHub firmware updates

Local 0.9.0-rc.1 adds Stable/Dev channels and mandatory publisher signatures.
Owner authorized prerelease publication and installation; Stable Latest remains
0.8.2. See
[SIGNED_OTA.md](SIGNED_OTA.md) for key custody, migration, packaging and test gates.

The development firmware implements opt-in updates from a **public GitHub
repository**, with manual Check/Install controls and optional daily automatic
installation. Both automatic settings default off. From v0.4.2 the project
repository is prefilled as `sideout909/WX-Link`; saved custom sources
remain supported. No GitHub token is needed. The first OTA transfer/boot was
followed by an unexpected owner-reported rollback to v0.4.0; final acceptance
and unattended update validation remain open.

v0.7.5 packages the current firmware for the renamed WX-Link repository.
The owner reports exercising the preceding firmware over the last 24 hours.
USB startup checks and owner use do not complete interrupted-transfer or
deliberate failed-boot hosted OTA acceptance tests.

## 0.8.2 startup validation fix

The owner's OTA attempt to 0.8.1 reported `Settings validation rejection (HTTP 409)`
and rolled back to 0.8.1-rc.1. The pending-boot save lock ran before semantic
validation, contradicting startup probes expecting HTTP 400 for invalid settings.
0.8.2 validates the candidate before checking the save lock. Valid requests still
receive HTTP 409 during boot confirmation; no settings are persisted by the probes.
Startup checks now test that lock explicitly while an OTA image is pending.
Build and host checks pass. The owner device installed the public 0.8.2 image
through OTA, passed portal startup checks, confirmed the image at 33 seconds and
remained on 0.8.2 beyond 100 seconds without rollback. Interrupted-transfer and
deliberate failed-boot tests remain open; see [patch notes](RELEASE_0.8.2.md).

## 0.8.1 Windy patch

Owner confirmed the Windy station online after the TLS fix and approved stable
0.8.1. That release was superseded by 0.8.2 after its OTA startup validation
regression was reproduced. Check now offers 0.8.2. The retained Windy fix uses verified
GTS Root R1 for Windy; certificate/hostname checks remain enabled. New protected
health diagnostics separate DNS, HTTP, TLS, last attempt and last acceptance.
Other services keep their existing trust. No 0.8.0 release asset was replaced.

## 0.8.0 stable release and candidate upgrade

The owner approved stable 0.8.0 after using the locally installed 0.8.0-rc.1
candidate. This historical release is superseded by 0.8.2 through Check now;
no USB flash is needed. The updater/package tools accept bounded
`-rc.N` identifiers, order final 0.8.0 after 0.8.0-rc.N and keep GitHub's stable
`releases/latest` lookup. A prerelease therefore does not become the normal
stable OTA target. Existing stable numeric versions and strict release-URL,
board/image/hash/rollback checks remain in force.

## Owner setup

1. On the setup page, open Updates. The project source is prefilled in newer
   firmware; older firmware needs `sideout909/WX-Link` entered. Save
   and restart if changing it; this does not enable checks or installations.
2. Check now retrieves the latest compatible release manifest. A normal stable
   release needs the two named assets below; a draft/prerelease is not the
   normal GitHub `releases/latest` target.
3. Install update and restart appears only for a newer compatible version.
   Automatic checking is a separate opt-in checkbox; automatic installation
   requires both checkboxes. Automatic installation waits during NWS watches/
   warnings or missing/stale/unverified NWS status when a location is enabled.

Through installed 0.8.2, HTTPS plus the manifest hash provide integrity with
the repository as authority. Local 0.9.0-rc.1 additionally requires a trusted
publisher signature; changing repositories cannot change trust. No Secure Boot
or security eFuses are enabled. See SIGNED_OTA.md before packaging/distribution.

## Release preparation

The board-specific image is an application-only binary, never the full-flash
hardware backup. NVS credentials and local captures are not release assets.

```bash
bash tools/pio.sh run -d firmware
bash tools/test-firmware.sh
python tools/package-firmware-release.py --repo OWNER/REPOSITORY \
  --signing-key /secure/path/publisher.key
```

From the repository root, the version lives in
`firmware/include/firmware_version.h`. Bump its three-number
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

From v0.4.2 the project repository `sideout909/WX-Link` is the
default for new settings and migrated empty repository fields. A saved custom
repository is retained. Clearing the field restores the project default on
save. Daily checks and automatic installation remain separate opt-in settings;
manual **Check now** works with both options off.

The setup page tracks an action's current updater message rather than leaving
its initial queued acknowledgement visible after completion. Lookup failures,
availability, download progress and startup confirmation replace that message.
During a restart, reconnect to the page to verify the running version.

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
paths follow [Espressif's OTA rollback mechanism](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/ota.html).
The owner initially observed v0.4.2 running, then reported reversion to v0.4.0.
This establishes transfer/boot and a fallback, not successful final confirmation.
The exact failure/reset cause remains unconfirmed without startup diagnostics.

The v0.4.3 correction waits for actual main-loop HTTP service before probing,
allows at most three portal self-test attempts, and reports the failed stage
and HTTP result without secrets. Settings saves are rejected while boot
confirmation is pending; authentication/CSRF and invalid-input rejection still
run before that guard. The original 30-second confirmation and 90-second
rollback thresholds and watchdog remain enabled. A successful test must show
**Updated firmware passed startup checks**, remain on the new version beyond
90 seconds, and survive a later controlled restart. Interrupted transfers and
deliberate failed-boot recovery need separate tests.
USB recovery remains available through the documented hardware profile.

## Before unattended use

Use a separate test release/repository and a higher version to exercise a
complete installation. Verify settings, history/rain, display/touch, portal,
radio, NWS and sharing after restart. Exercise interruption, wrong hash,
wrong board/layout, older version, and a failed first boot/rollback. Measure
heap and receiver behavior while OTA and NWS TLS requests overlap. Keep
automatic installation off until these acceptance checks pass.


v0.7.3 startup self-tests use SDK loopback (127.0.0.1) to reach the actual HTTP
server independently of AP/STA route changes during mDNS initialization. All
authentication/CSRF/schema tests and boot deadlines remain. A passing USB bench
probe is not a replacement for the corrected hosted OTA acceptance test.

## Repository rename — October 22, 2026

The canonical project source is now `sideout909/WX-Link`. v0.7.5
defaults to that source and migrates a saved `sideout909/CYD-Weather-Link`
source to it on load/save. Custom fork sources and update opt-in flags are
preserved. The installed v0.7.4 predates this migration; its Updates field can
be changed manually, or it will migrate when v0.7.5 is installed. Before checking for v0.7.5 on older firmware, set
Updates → Public GitHub owner/repository to `sideout909/WX-Link` and save.
The release manifest names the canonical repository, and older firmware verifies
that its download URL matches the configured repository. The rename alone did
not install firmware; v0.7.5 is the subsequent release.
