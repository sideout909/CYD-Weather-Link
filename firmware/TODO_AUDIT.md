# Remaining work — stable 0.8.2, October 22, 2026

Published GitHub Latest remains **0.8.2**; installed prerelease is **0.9.0-rc.1**.
Signed OTA/startup confirmation and preference restoration passed. This is the current
TODO index; dated work/validation logs retain historical results; see [SIGNED_OTA.md](SIGNED_OTA.md).

## Completed

- Atlas/CC1101 reception, protected setup/discovery, dashboard and bounded history/export.
- Owner-confirmed APRS.fi, Weather Underground, Weathercloud, Windy and PWSWeather ingestion.
- Normal hosted 0.8.2 OTA: startup checks passed, image confirmed at 33 seconds,
  still running beyond 100 seconds. Settings validation/save-lock deadlock fixed.
- Factory-reset implementation, metric/imperial presentation, Barometer label,
  configurable hostname, web tab/health cleanup and NWS AWC pressure fallback.
- Seven themes, clock/date settings, packet age/status, phase-aware forecast moons,
  layered sun graphics, setup guidance, header navigation and graph/forecast timeout.
- Normal Dim/Off wake, themes and forecast moons owner-confirmed. Sound removed.
- Metric/custom-hostname persistence and mDNS checked on bench; owner defaults restored.
- Experimental lux clouds implemented/calibrated on owner unit; default off elsewhere.
- Local touch-target overlay delivered; physical review remains below.

## Owner checks — when convenient

- Test interrupted OTA download; report recovery behavior. Deliberate failed-boot
  rollback needs a separate coordinated bench procedure, not a routine update.
- If overnight dim/touch lockup recurs, leave the unit powered on for inspection
  before resetting. Normal short-term wake checks do not resolve that incident.
- Check updated header touch targets, Settings browser guidance, Barometer fit and
  metric/imperial layout on desired pages/themes. See [controls](CONTROLS_CHECKLIST.md).
- PWSWeather and Windy ingestion are both owner-confirmed; no setup task remains.
- Compare LUX/LUX* sky estimates against actual cloudy transitions over several days;
  recalibrate only in clear direct sun, solar elevation at least 15 degrees. Owner
  expects cloudy days and is leaving the existing device powered overnight.
- Factory reset is optional acceptance only when ready to erase settings/history
  and reconfigure. Owner plans to test later; no real destructive reset has been run
  against owner data.

## Engineering and joint validation

- Diagnose overnight dim/touch failure on recurrence: check live clock/web/RF,
  uptime, brightness/idle age and touch counters before reset. Owner requested
  no deep dive without recurrence evidence; no cause/fix is established.
- Compare CC1101 counts/fields with RTL-SDR; review antenna/interference and measure
  debug-header supply/headroom under simultaneous radio/display/Wi-Fi load.
- Extended RF/TLS/flash soak, transmitter silence/unchanged-data recovery, natural
  midnight rain, DST/clock changes and network outages. Power-loss/history stress
  is lower priority at owner's request, deferred rather than passed.
- Separate CWOP registration/routing/MADIS ingestion and field-level accuracy at
  each destination. Connected/sent alone is not ingestion confirmation.
- Exercise deliberate failed-boot rollback with a controlled recoverable image.
  Normal hosted OTA passed; interruption/fault testing is still separate.
- Review seasonal/cloudy lux model accuracy and physical layout feedback.

## Next release and deferred features

- **Stable / Dev selector — implemented locally in 0.9.0-rc.1.** Stable migration
  default, explicit Dev opt-in, saved selection and no downgrades. Browser/model
  checks and target-device lookup/persistence acceptance pass.
- **Signed OTA — implemented locally.** Trusted embedded public key, ECDSA P-256/
  SHA-256 envelope, fail-closed verification and required matching-key packaging.
  Actual firmware verifier passes host mbedTLS tamper/wrong-key tests. Offline
  publisher-key backup and extended resource/soak checks remain pending. Bootstrap and
  signature-enforced subsequent OTA acceptance passed. Rotation/recovery still need a
  dual-key transition implementation. No Secure Boot/eFuse/flash-encryption changes.
- **Optional lightning on Home — implemented, storm validation pending.** Alternate Barometer and
  lightning every five seconds while activity is recent; include last-strike
  estimated distance/age. After thirty minutes with no real lightning-data
  change, hide lightning and show only Barometer. Identical packets do not reset
  expiry; absent capability stays hidden. Atlas protocol notes say miles, converted
  to km for metric; reserved codes and counter/coverage semantics need validation.
  See [selected prototype](../concepts/lightning/selected-v2/index.html).
  See [implementation and tests](LIGHTNING.md). Validate real strike/distance
  agreement and reserved codes; session-only history restarts after reboot.
- **Multiple sensor sources.** Explicitly assign temperature/wind/pressure devices
  to one station with per-source/per-field freshness and provenance. Deferred.
- **More station families.** Use community Connect captures, review native identity,
  fields/units/integrity and receiver profiles. Atlas is operational; Iris/Notos
  remain discovery previews. Connect review reran 16 public waveforms/40 callbacks
  successfully; see [review and priorities](CONNECT_WAVEFORM_REVIEW.md). No new
  station enabled. Decoder-library presence is not compatibility.
- Optional D710 electrical/serial output and local barometer; verify interfaces/pins.

## Separate capture project

Documents/WXLink Connect has its own progress and hardware acceptance gates.
Do not mix its NVS/storage, capture evidence or OTA source with production WX Link.
Nothing in this audit validates or modifies Connect.

Implementation details: [FEATURE_QUEUE.md](FEATURE_QUEUE.md).
Evidence: [VALIDATION.md](VALIDATION.md), [WORK.md](WORK.md).
