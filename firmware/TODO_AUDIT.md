# Remaining work — stable 0.8.2, October 22, 2026

Published GitHub Latest and installed device are **0.8.2**. This is the current
TODO index; dated work/validation logs retain historical results.

## Completed

- Atlas/CC1101 reception, protected setup/discovery, dashboard and bounded history/export.
- Owner-confirmed APRS.fi, Weather Underground, Weathercloud and Windy ingestion.
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
- Verify PWSWeather ingestion with an account if desired. Windy is already confirmed.
- Compare LUX/LUX* sky estimates against actual cloudy transitions over several days;
  recalibrate only in clear direct sun, solar elevation at least 15 degrees.
- Factory reset is optional acceptance only when ready to erase settings/history
  and reconfigure. No real destructive reset has been run against owner data.

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

- **Next release: Stable / Dev update selector.** Persist user choice in Updates;
  default existing/new devices to Stable, require explicit Dev prerelease opt-in,
  clearly label channel/version, and retain compatibility/hash/rollback guards.
  Define return-to-Stable and downgrade behavior before implementation. TODO only;
  no channel code yet.
- **Signed OTA.** Independently verify publisher signatures; plan key protection,
  rotation/recovery and rejection tests. Keep USB/custom firmware available;
  no Secure Boot, flash encryption or permanent eFuse changes. Want only.
- **Multiple sensor sources.** Explicitly assign temperature/wind/pressure devices
  to one station with per-source/per-field freshness and provenance. Deferred.
- **More station families.** Use community Connect captures, review native identity,
  fields/units/integrity and receiver profiles. Atlas is operational; Iris/Notos
  remain discovery previews. Decoder-library presence is not compatibility.
- Optional D710 electrical/serial output and local barometer; verify interfaces/pins.

## Separate capture project

Documents/WXLink Connect has its own progress and hardware acceptance gates.
Do not mix its NVS/storage, capture evidence or OTA source with production WX Link.
Nothing in this audit validates or modifies Connect.

Implementation details: [FEATURE_QUEUE.md](FEATURE_QUEUE.md).
Evidence: [VALIDATION.md](VALIDATION.md), [WORK.md](WORK.md).
