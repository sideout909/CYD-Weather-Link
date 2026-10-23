# WX Link 0.9.0-rc.1 — prerelease

Owner authorized prerelease publication and device installation. Stable Latest
remains 0.8.2. This prerelease requires further overnight/storm validation.

- Stable/Dev update channels with Stable default and explicit Dev opt-in. Returning
  to Stable waits for a newer stable release; no downgrades.
- Publisher-signed OTA: embedded public trust, ECDSA P-256/SHA-256 verification,
  signed metadata/image digest, matching-key packaging. No unsigned fallback.
- Repository field starts read-only; checkbox enables editing. Reload locks it.
- Header keeps YOUR STATION. YOUR DATA., uses WX Link, replaces hardware names
  with Live conditions · Local history · Weather sharing, and places GitHub below IP.
- Optional Atlas lightning/Barometer tile alternates every five seconds during
  recent activity, with last-strike distance/age and a ten-minute count. Thirty
  minutes unchanged returns to Barometer; identical packets do not renew it.
  Missing capability stays hidden; unknown distance stays unknown. Session-only
  history, conservative reset/wrap/gap handling and partial-count indication.
  See [LIGHTNING.md](https://github.com/sideout909/WX-Link/blob/main/firmware/LIGHTNING.md); real storm/distance comparison remains pending.
- Removed Atlas-specific Missing types from Outdoor/status.
- PWSWeather ingestion now owner-confirmed. Connect waveform evidence reviewed;
  no additional production station support enabled by these replay tests.

Build, host, browser and actual mbedTLS signature-verifier tests pass. Lightning
model/native adapter checks include timing, coverage, wrong-station rejection,
mi/km and packed-font fit. App-only release package contains notices and signed
manifest; no credentials, NVS or captures. Hardware installation/OTA results are
recorded separately in VALIDATION.md. Interrupted-transfer, deliberate failed-
boot, storm accuracy and overnight dim/touch acceptance remain open.

The first upgrade from 0.8.2 cannot itself enforce a signature; the new firmware
requires a trusted publisher signature for subsequent OTA checks. Back up the
publisher key outside this repository; no key or token is needed on user devices.
Returning from Dev to Stable waits for a newer signed stable release, never a
downgrade. Existing users can still use USB/custom builds; no eFuses change.
