# Outstanding work audit — October 21, 2026

Quick documentation/source review before preparing the separate WXLink Connect
handoff. Baseline is local v0.7.3, installed over USB. This audit does not claim
new soak, RF, power or provider tests. No firmware/device settings changed.

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

1. Corrected hosted OTA installation: stay on the new version beyond 90 s,
   controlled reboot, interrupted download and deliberate failed-boot rollback.
   Auto-install remains owner-controlled; no new release is published here.
2. CC1101 versus RTL-SDR counts/fields, antenna/interference observations, and
   debug-header voltage/headroom with radio/display/Wi-Fi active.
3. Prolonged TLS/flash/RF load; transmitter silence/unchanged-data recovery;
   natural-midnight rain, clock/DST transitions, abrupt power loss, extended
   network outages and a multiday soak.
4. Separate CWOP registration/routing/MADIS ingestion and field-level accuracy
   at every destination. A connected session is not proof of ingestion.
5. Owner checks for newer theme persistence, physical Night brightness/wake,
   changed Wi-Fi touch target and forecast moon appearances. v0.6.0 controls
   were owner-reported passed; do not invent individual newer confirmations.
6. Name/IP display and lookup after reconnect/on setup Wi-Fi; networks blocking
   multicast use numeric IP. Configurable names for multiple units are future work.

## Remaining feature roadmap

- Metric/imperial presentation setting, persisted through restart and shared by
  device/web views; preserve storage/export protocol units.
- Rename the main-screen "Baro" label to "Barometer"; check tile fit across themes.
- Other station families: Iris/Notos are detection previews, not operational
  support. Require real captures and field/unit/integrity/profile validation.
- Windy exporter, then potentially PWSWeather.
- Optional D710 electrical/serial and local barometer with verified interfaces.
- Deferred calibrated lux-to-sky inference; never call it implemented.

## Separate follow-up workspace

The owner requested Documents/WXLink Connect as a local-model handoff for
community station capture. That workspace must not change production weather
firmware, publish experimental updates to its OTA source, or silently upload
weather from unverified stations. Capture evidence is not a compatibility claim.
See WORK.md, VALIDATION.md and CONTROLS_CHECKLIST.md for original evidence.
