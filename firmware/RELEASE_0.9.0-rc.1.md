# WX Link 0.9.0-rc.1 — local candidate

Not published, installed or production-validated. Published/device stable is 0.8.2.

- Stable/Dev update channels with Stable default and explicit Dev opt-in. Returning
  to Stable waits for a newer stable release; no downgrades.
- Publisher-signed OTA: embedded public trust, ECDSA P-256/SHA-256 verification,
  signed metadata/image digest, matching-key packaging. No unsigned fallback.
- Repository field starts read-only; checkbox enables editing. Reload locks it.
- Header keeps YOUR STATION. YOUR DATA., uses WX Link, replaces hardware names
  with Live conditions · Local history · Weather sharing, and places GitHub below IP.
- PWSWeather ingestion now owner-confirmed. Connect waveform evidence reviewed;
  no additional production station support enabled by these replay tests.

Build, host, browser and actual mbedTLS signature-verifier tests pass. Signed
local artifacts are in dist/v0.9.0-rc.1. Target-device lookup/persistence, signing
resources, bootstrap/subsequent enforced OTA and key backup remain release gates.
Future rotation needs a dual-key transition implementation. See SIGNED_OTA.md.
The owner device stays on 0.8.2 for overnight observation; no flash/publication.
