# Ordered implementation queue — October 22, 2026

Owner requested this order and later authorized USB/device updates again.
Owner approved publication after 0.8.0-rc.1 use. Stable version is 0.8.0;
remaining field-specific validation gates below stay explicit.
Save actual checks and next steps here at each feature boundary.

1. **Factory reset — software complete; hardware acceptance pending.** Protected typed-confirmation webpage action;
   deliberate physical recovery for forgotten passwords; project-only settings,
   credentials, weather history/cache reset. Durable pending marker, retry at
   boot before services, no whole-flash/NVS partition format. No real reset run.
2. **Metric/imperial — software complete; physical layout check pending.** Persisted presentation setting; display,
   browser, forecast and trends agree. Raw storage/export protocol units stay
   unchanged. Test conversions, missing fields and existing default migration.
3. **Baro → Barometer — software complete; physical check pending.** Main-screen label and web label; fit tile.
4. **PWSWeather uploading — software complete; live acceptance pending.** Opt-in
   station/key controls, protected persistence, status, five-minute cadence and
   bounded backoff. SC05_X firmware build, host and browser checks pass.
   Live provider acceptance remains unverified. See `PWSWEATHER.md`.
5. **Windy uploading — software complete; live ingestion pending.** v2 HTTPS,
   Bearer station password, safe status, five-minute cadence/backoff/retry_after.
   Host/browser checks pass. See `WINDY.md`.
6. **Configurable hostnames — implemented.** Validated persisted DNS labels,
   mDNS, DHCP hostname, display/web URLs and IP fallback; default wxlink preserved.
   Multi-sensor field sources remain a separate deferred idea.
7. **Web cleanup — implemented.** Connection health first, collapsed wiring guide
   last; Sun & Moon only in Overview. Factory reset remains in Wiring & Health.
8. **NWS pressure — investigated and fixed with fresh AWC fallback.** Primary
   api.weather.gov KRNP data lagged four hours; AWC METAR was current. Identity,
   age, range and pressure-type guards preserved. See `NWS_PRESSURE.md`.
9. **Lux clouds — experimental implementation.** Explicit clear-sky calibration,
   solar elevation/season-aware reference, smoothing and stable-sample gate;
   LUX on display / LUX* on web only when valid, NWS fallback otherwise.
   Owner authorized today's clear-day data for calibration. See `LUX_CLOUDS.md`.

## Additional request for the next local build

APRS login software identity and weather-packet comment now use WX-Link in
the installed prerelease, with its current firmware version in the login. Build
and reporting host checks pass; normal live reporting resumed after USB update.

## Existing local v0.7.6 work

Owner-marked header split, combined Outdoor/status, browser-settings guide and
layered sun/cloud weather graphics pass build/tests and are installed in
0.8.0-rc.1. Owner physical acceptance remains pending.
Stable 0.8.0 publication was subsequently approved by the owner.

## Local evidence and next work

Factory reset: simulated backend checks confirmation, missing files, failure at
each deletion boundary, interrupted completion/retry and preservation of an
unrelated Connect file. Browser checks cancel/no request, wrong phrase disables
confirmation, and one explicit mocked POST with CSRF. Startup self-test includes
only unauthorized/CSRF/invalid-confirmation probes, never a real reset. No
physical reset or wipe performed; the prerelease firmware was installed. Reset is logical deletion, not forensic
credential erasure; rotate credentials before disposing of hardware.

Metric: C++ tests cover freezing/boiling/negative/zero and pressure/rain/wind.
Browser test saves/reloads metric, checks C, km/h, mm, hPa, trend and forecast
units, and retains raw API Fahrenheit. Dedicated NWS wind descriptors convert;
provider narrative is kept verbatim. Existing settings default to imperial.
SC05_X build, host suite and browser suite pass for reset/units.

Barometer label fits the original tile font in the host check; build and host
suite pass. PWSWeather implementation is now present. Windy is now integrated; live ingestion remains owner acceptance.
USB access was reauthorized for the 0.8.0 prerelease bench checks.
