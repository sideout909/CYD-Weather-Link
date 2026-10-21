# Current development state

October 21, 2026. Owner direction: finish core firmware for the owned PanelLan
ZX2D80CE02S-V10, focus Atlas + CC1101, preserve Home v2, and finish browser setup.
SD logging and D710 serial are optional. PC RTL-SDR recording stays stopped.

## Running board

The board now runs v0.3.2. It includes the callsign/speaker/APRS header,
Link status details, Atlas stale-cycle/no-signal monitoring, persistent APRS/CWOP
sessions and opt-in GitHub OTA, alongside the earlier map/security features.
USB upload preserved Wi-Fi/location/custom admin settings. Display, portal,
credential separation, checkpoint diagnostics and live NWS recovered.
A short on-device replay verified Current → Data stale when only type 37
continued; it was cleared and the board was returned to Home with no fake data.
The v0.3.1 patch fixes repeated setup saves and reveals invalid fields across
tabs, accepts fractional saved elevation, enlarges the callsign and uses spaced
+ / X connection indicators. Browser regressions and USB upload passed.
Both automatic update settings default off. The GitHub repository and v0.3.1
release are published; no device update setting was changed by publishing.
Hosted OTA/rollback and live weather ingestion remain unverified. The owner
configured APRS and its acknowledged connection recovered after the last upload.

## Implemented core

- Owned N8R2 hardware profile, private original flash backup, display/touch and
  bidirectional wrapping Home/Trends/Forecast swipes.
- Home v2 antialiased DejaVu geometry/themes; later owner wording overrides the
  fictional study. RAIN TODAY resets at configured local midnight, independently
  of packet arrival; rolling hour/24-hour rain remains in details/exports.
- Sparse Atlas 37/38/39 fields and independent ages, received wind peak over
  five minutes, bounded minute history, CRC LittleFS checkpoints and rain state.
- Atlas-selected rtl_433_ESP/CC1101 input and SI/native JSON adapter; USB replay
  cannot upload or overwrite production checkpoints.
- Exact wiring from the owner's module photograph. CC1101 VCC/GND use debug
  pin 2 (+3.3 V) / pin 7 (GND) as the owner-selected bench option. Spare supply
  capacity is unmeasured; a separate regulator remains an alternative.
- Protected AP/LAN setup, saved Wi-Fi/location/timezone, separate visible AP key
  and private custom admin login. Legacy migration preserves the admin password.
- Embedded local Leaflet, ZIP/ZIP+4 centering, explicit dropped-point confirmation
  and USGS ground elevation. Manual coordinates/elevation remain available.
- Verified-TLS NWS forecast, 30-second alert scheduling with cache/backoff rules,
  downloaded sun times/moon phase and forecast-derived day/night weather icons.
- Opt-in Weather Underground, Weathercloud and APRS/CWOP transports. Real remote
  account ingestion remains unverified; replay and stale cycles cannot upload.
- v0.3.0 adds Current / Data stale / No signal distinctions and session indicators,
  authenticated/CSRF-protected GitHub update controls, inactive-slot/hash checks
  and deferred startup confirmation/rollback. Actual hosted OTA is unverified.

## Validation and next checks

Commands and evidence are in README.md and VALIDATION.md. Host checks cover the
weather model, daily rain/DST, Atlas JSON adapter, sensor health, credential
privacy, APRS login parsing and OTA release policy. Browser checks use synthetic
responses and mocked map tiles/providers. Build tools live outside the path
with spaces at ~/.local/share/cyd-weather-link/venv. Only one USB holder at a time.

Owner confirmed display/touch/pages, NWS setup, sunset-to-moon behavior and
watch/warning appearance. Measurement/link touch targets and phone map behavior
have a checklist in CONTROLS_CHECKLIST.md. Screenshots must never export the
physical setup page's Wi-Fi access key.

The owner connected CC1101 and first live Atlas reception is confirmed.
v0.3.2 corrects the native `uv` field mapping (retaining desktop `uvi` support);
the missing field previously kept full-cycle health from becoming Current.
Three minutes of live observation now retain Current with 18 accepted frames,
33 repeats, no queue drops/rejections/serial errors, and Wi-Fi/NWS/APRS active.
An APRS report send is observed; remote ingestion is still unverified.
Next hardware checks: compare live decoding with the SDR reference and validate
loaded debug supply voltage, simultaneous TLS/flash/RF load, transmitter freeze/silence recovery, natural-midnight rain,
remote service ingestion, power-loss recovery and a multiday soak.

GitHub repository: https://github.com/sideout909/CYD-Weather-Link. Local
release packaging targets this repository. Device update settings remain
owner-controlled; publishing a source commit does not enable automatic updates. Test hosted install, interruption and rollback
before unattended updates. Windy, optional D710, sound and pressure are later work.


Owner selected GNU GPL version 3 for original project code. The root LICENSE
and README record GPL-3.0-only; third-party terms are retained separately.
The initial source upload is complete, and v0.3.1 release packaging includes
project and dependency license notices. No device credentials are release assets.
Hosted OTA install/rollback validation remains open; keep auto-install off.


## Deferred TODO: other weather stations

- [ ] Verify the owner's Atlas using the physical CC1101 before expanding decoders.
- [ ] Add a bounded RF discovery scan that suggests a station after several
  separate valid transmissions, then requires user selection/confirmation.
- [ ] Keep manual station setup as an alternative; tune scan time/confidence
  with real captures instead of assuming Atlas timing for every station.
- [ ] Generalize identity binding, fields/units, freshness, gust and rain behavior
  for supported protocol families; do not silently mix neighboring devices.
- [ ] Prepare community build/report/capture instructions and recruit volunteers
  with other stations. Review sanitized data into regression fixtures and record
  actual hardware compatibility separately from decoder availability.

Full proposed flow and evidence requirements are in DESIGN.md. This milestone
is planning only; the running firmware and decoder selection remain unchanged.
