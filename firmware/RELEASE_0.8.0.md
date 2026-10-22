# WX Link 0.8.0

Stable release approved by the owner after successful use of the 0.8.0-rc.1
candidate on the PanelLan SC05_X. Firmware behavior is unchanged from that
candidate apart from the stable version identifier. Public assets are application-
only; full-flash backups and credentials are never published.

## Changes

- Scoped factory reset from protected web setup, with typed confirmation and
  deliberate physical password-recovery sequence. Durable marker retries an
  interrupted reset before workers. Firmware/partition/eFuse state is preserved.
- Metric/imperial display, detail, chart, discovery and forecast presentation.
  Stored/exported raw observations and service protocol units stay unchanged.
- BAROMETER label and explicit nearby pressure source/type/age. Fresh NWS AWC
  METAR fallback fixes observed primary NWS API lag without weakening age guards.
- Independent opt-in PWSWeather and Windy.com v2 uploaders, protected retained
  keys/passwords, safe status, five-minute cadences and failure backoff. Windy
  uses station-password Bearer authentication, not a legacy account-key endpoint.
- Saved per-device hostname for mDNS/DHCP and device/browser settings links,
  default wxlink, with numeric IP fallback.
- Connection health first in Wiring & Health; collapsed wiring guide last.
  Sun & Moon appears only on Overview, not every setup tab.
- Experimental calibrated local daylight cloud estimate. Solar position handles
  hour/latitude/season, excludes low sun/night, requires fresh stable RF light,
  and labels it LUX on the display, LUX* on the webpage; otherwise NWS. Forecasts remain NWS.
- Owner-marked two-row header touch split, merged Outdoor/status, prominent
  browser-settings guidance and layered sun/cloud art matching the moon approach.
- APRS software identity and packet comment use WX-Link.
- Release-candidate version ordering preserves a clean upgrade to final 0.8.0.

## Verification and remaining gates

Build, host suite, synthetic browser checks and actual device HTTP/browser checks
pass. Hardware accepted fresh AWC pressure. Metric/custom hostname persisted and
resolved across reboot; original preferences were restored. Owner-confirmed clear
sky enabled a median calibration from 153 recorded daylight minutes. Existing
credentials/history and upload flags were preserved; new destinations stay off.

Still verify actual destructive reset when the owner is ready, physical control
acceptance, real PWSWeather/Windy ingestion and cloudy/seasonal classifier accuracy.
The reported overnight dim/touch incident and interrupted/failed-boot OTA tests
remain open. No physical cause or fix for that overnight incident is claimed.

The new transparent project icon is included as a separate downloadable PNG.

See [validation](VALIDATION.md), [controls](CONTROLS_CHECKLIST.md),
[Windy](WINDY.md), [PWSWeather](PWSWEATHER.md), [pressure](NWS_PRESSURE.md) and
[local sky estimation](LUX_CLOUDS.md). Local lux inference and external pressure
are display features and are never uploaded as local weather observations.
