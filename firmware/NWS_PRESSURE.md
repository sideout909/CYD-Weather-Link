# Nearby NWS barometer fallback

Pressure is display-only and never passed off as local telemetry or uploaded.
The main label is Barometer. Details identify station, provider, pressure type
and observation age. Metric uses hPa; imperial uses inHg.

## October 22 investigation and fix

At about 17:19 UTC the device and a direct PC request both received KRNP's
13:15 UTC observation from api.weather.gov. Its pressure was valid (102510 Pa,
QC V), but its age exceeded the unchanged 90-minute display limit. The nearby
KFNT/KMBS latest observations and KRNP recent-observation list showed the same
lag; asking for require_qc=false did not resolve it. This establishes an API
path delay, not a station/transmitter failure or evidence of a security incident.

[NWS API guidance](https://www.weather.gov/documentation/services-web-api)
notes upstream MADIS/QC delays and variation among observation providers. Its
normal stated delay does not explain this entire four-hour incident; no root
cause within NWS infrastructure was established.

The [NWS Aviation Weather Center API](https://aviationweather.gov/data/api/)
and NWS METAR text feed both had KRNP's 17:15 observation while the other API
lagged. The implementation now falls back to AWC after the normal nearby-station
request fails validation. It batches up to three known four-character station IDs
in one HTTPS request, validates identity/time/numeric range, and prefers the
nearest eligible station. The original api.weather.gov request is retried on the
next normal cycle, so the fallback is not permanent.

The [AWC schema](https://aviationweather.gov/data/schema/openapi.yaml) defines
`slp` and `altim` in hPa and `obsTime` as Unix seconds. Prefer sea-level pressure;
if absent use altimeter setting and explicitly label that type. Altimeter setting
is not claimed to be measured MSLP or the pressure at the owner's sensor height.

Successful refreshes are at least ten minutes apart, respecting cache/retry
intervals; failed normal cycles rotate nearby stations at one-minute intervals.
AWC requests use certificate verification, a custom User-Agent, timeouts and
filtered bounded JSON storage. Missing, malformed, bad-unit, out-of-range,
future and 90-minute-old samples remain unavailable. No stale sample is relabeled
as current. Status retains the failed primary station/timestamp and primary error
alongside the accepted fallback's provider, sample and pressure type.

Bench result: v0.8.0-rc.1 accepted KRNP 17:15 AWC pressure as 102510 Pa, reported
`NWS AWC`, `Altimeter setting`, and fresh/current while retaining the primary
13:15 diagnostic. A fresh TLS/HTTP fallback was exercised on the real ESP32.
Host checks cover units, null/range/future/stale handling and SLP preference.
