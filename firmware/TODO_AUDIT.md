# Outstanding work audit — October 21, 2026

Quick documentation/source review before preparing the separate WXLink Connect
handoff. Baseline is local v0.7.3, installed over USB. This audit does not claim
new soak, RF, power or provider tests. Updated October 22 with the release/OTA
result and local feature requests; historical baseline entries remain below.

## Already completed; do not reopen as feature TODOs

- Atlas/CC1101 reception, protected setup/discovery, dashboard, recording/export.
- Owner-confirmed APRS.fi, Weather Underground and Weathercloud ingestion.
- Date/clock, main header/status updates, packet age, Receiving wording.
- Seven shared themes, Night low backlight/soft NWS green, forecast moon phases.
- Sound removed from this hardware; future audio notes are archived.
- wxlink.local and numeric IP fallback; owner confirmed the name works.
- v0.7.3 loopback portal startup checks passed on attempt one after the earlier
  AP-route probe failure. This is not a hosted OTA acceptance test.

## Remaining priority acceptance checks

- NWS barometer incident investigated/fixed in 0.8.0-rc.1: api.weather.gov's
  KRNP sample lagged four hours; NWS AWC had a fresh METAR. Added a validated
  AWC fallback and explicit altimeter/source/age diagnostics. Physical device
  accepted the fresh sample; see NWS_PRESSURE.md. No age guard was relaxed.

- Investigate owner-reported v0.7.5 overnight dim/wake failure: screen remained
  dim and touchscreen did not respond. Owner reset before inspection. After
  reset, USB status showed a running display/main loop, three touch events,
  working Wi-Fi/radio/APRS and restored history/archive; this cannot establish
  the pre-reset cause. No clear dim/wake logic cause found in a brief review.
  Owner requested no deep investigation yet. On recurrence leave powered on;
  inspect USB status/uptime, brightness/idle age, touch-event count during taps,
  live clock/web responsiveness and RF activity before resetting. No firmware
  fix, touch-controller fault or whole-device freeze has been established.

1. Normal hosted OTA to v0.7.5 is owner-confirmed, including the requested
   beyond-90-second and restart checks. Interrupted download and deliberate
   failed-boot rollback remain untested. v0.7.5 is published as Latest; automatic
   installation remains owner-controlled.
2. CC1101 versus RTL-SDR counts/fields, antenna/interference observations, and
   debug-header voltage/headroom with radio/display/Wi-Fi active.
3. Prolonged TLS/flash/RF load; transmitter silence/unchanged-data recovery;
   natural-midnight rain, clock/DST transitions, extended network outages and
   a multiday soak. Owner deprioritized power-loss/history-retention stress
   testing because external services retain uploaded observations; recovery
   checks remain deferred rather than claimed passed.
4. Separate CWOP registration/routing/MADIS ingestion and field-level accuracy
   at every destination. A connected session is not proof of ingestion.
5. Owner confirmed themes, normal Dim/Off sleep/wake and forecast moon
   appearance on October 22. Touch targets remain under review; a local
   source-derived overlay was requested. The separate overnight dim/touch
   failure remains open despite normal short-term wake checks passing.
6. Owner confirmed hostname access on setup Wi-Fi. Home-LAN hostname access
   was previously confirmed; additional reconnect regression remains optional.
   Numeric IP remains the fallback when multicast is unavailable.

## Remaining feature roadmap

Implementation order/status: [FEATURE_QUEUE.md](FEATURE_QUEUE.md). Items with
local software completion still require physical acceptance before release.

- Next release: offer a persisted Stable / Dev update-channel selector in Updates.
  Default existing/new devices to Stable; require explicit opt-in for development
  prereleases. Clearly label the selected channel and retain image compatibility,
  hash and rollback checks. Planning only; no channel implementation yet.

- Signed OTA releases: verify a publisher signature using a trusted public key
  independently of GitHub HTTPS and the manifest hash. Cover image/version/board
  identity, signing-key protection, key rotation and rejected-signature tests.
  Preserve owner-controlled custom builds and USB reflashing; no Secure Boot,
  flash-encryption enablement or permanent eFuse changes. Want only, not implemented.
- User-accessible factory reset (implemented locally; hardware check pending): authenticated webpage action
  with explicit destructive confirmation, plus a deliberate physical recovery
  method for a forgotten admin password. Define reset scope visibly: Wi-Fi,
  admin/AP keys, station/location, sharing credentials, update/display settings
  and retained weather history. Regenerate initial setup credentials and return
  to first-run setup. Clear only this project's settings/files; do not erase
  unrelated flash or alter bootloader/partitions. Check reboot/power-loss behavior
  and ensure no retained secret is displayed or exported.

- Metric/imperial presentation setting (implemented locally), persisted through restart and shared by
  device/web views; preserve storage/export protocol units.
- Main-screen/web "Barometer" label implemented locally; host font fit passes,
  physical theme/layout check pending.
- Receive from multiple sensor devices and combine explicitly assigned fields
  into one station: separate temperature, wind and/or pressure devices. Let the
  user choose each source identity; retain per-source/per-field freshness and
  provenance so unrelated or stale measurements are not silently combined.
  Deferred planning only; owner separately restored configurable hostnames, now implemented in 0.8.0-rc.1.
- Other station families: Iris/Notos are detection previews, not operational
  support. Require real captures and field/unit/integrity/profile validation.
- PWSWeather/Windy v2 adapters implemented in 0.8.0-rc.1; live destination acceptance pending.
- Optional D710 electrical/serial and local barometer with verified interfaces.
- Experimental calibrated lux-to-sky inference is implemented in 0.8.0-rc.1; validate cloudy transitions and seasonal performance.

## Separate follow-up workspace

The owner requested Documents/WXLink Connect as a local-model handoff for
community station capture. That workspace must not change production weather
firmware, publish experimental updates to its OTA source, or silently upload
weather from unverified stations. Capture evidence is not a compatibility claim.
See WORK.md, VALIDATION.md and CONTROLS_CHECKLIST.md for original evidence.
