# Windy.com v2

Use [Windy's current API reference](https://stations.windy.com/api-reference)
and [registration guide](https://stations.windy.com/docs). This integration is
for Windy.com. Provider docs identify v2 as the replacement for the legacy API.

## Implemented locally

`windy_weather.h` creates an observation query for
`https://stations.windy.com/api/v2/observation/update`. Windy's embedded OpenAPI
schema was inspected from its API-reference page on October 22, 2026.

- Station ID: at most 30 characters; generated alphanumeric/hyphen/underscore IDs.
- Authentication plan: station password in an HTTPS Authorization Bearer header;
  account management API keys are not the upload credential.
- Cadence: at least five minutes between requests; failures need bounded backoff.
- Timestamp: explicit POSIX seconds from the observation, never fabricated current time.
- Fields: Celsius, m/s wind/gust, degrees direction, percent humidity, hourly rain
  in mm with coverage, and UV. No daily rain, inferred solar radiation or NWS pressure.
- HTTP 200 means accepted for processing; it does not prove map visibility.
  400/401/409/429 are not success. A 429 can include a retry_after timestamp.

Project checks additionally require live/fresh data less than 120 seconds old,
omit invalid fields, retain measured zeroes and exclude replay.

Host adapter tests exercise conversion, omissions, valid zero/100%, timestamp
bounds, IDs and response codes. See WINDY_TLS_FIX.md for the subsequent real-device
connection investigation after the owner configured Windy.

## Saved setup controls

Opt-in Windy fields now persist in project settings. Public configuration exposes
only the password-presence flag; blank input retains the private saved password.
The Sharing card configures uploads; the option defaults off. Passwords reject
control characters, spaces and non-ASCII bytes to protect the planned Bearer header.
Synthetic browser checks cover enable/ID/password submission and clearing inputs.

## Network worker and validation

The HTTPS worker uses certificate validation and a station-password Bearer header;
redirects are disabled. It rechecks live source/freshness before sending. HTTP 200
is labeled Accepted for processing, never proof of map ingestion. 400/401/409/429
have safe distinct status messages; arbitrary response text and credentials are
not logged. Failures back off from ten minutes to one hour. A valid retry_after
within 24 hours can extend that delay; invalid dates cannot disable indefinitely.
Success resets to five minutes. Monotonic scheduling handles timer wrap.

Host tests cover field conversion, missing/zero values, replay/future/stale
rejection, header-safe passwords, exact HTTP acceptance, retry bounds and timer
wrap. Browser tests cover enable/ID/password submission and clearing the password
input; startup probes check masking and reject invalid settings without saving.
The uploader is included in the SC05_X build. The owner has now configured Windy.
Initial real-device attempts found a TLS bundle-chain failure, not a DNS block
or a reported credential rejection. The 0.8.1-rc.1 fix trusts the verified public
GTS Root R1 directly and exposes safe connection codes; see WINDY_TLS_FIX.md.
Map/service-side ingestion remains distinct from accepted HTTP requests.


## October 22 TLS fix and first accepted upload

The 0.8.1-rc.1 candidate received HTTP 200 Accepted for processing on the real
ESP32 with the owner's configured account and fresh Atlas data. DNS/Pi-hole was
unchanged. This is the first confirmed HTTP acceptance for this integration;
the owner then confirmed the station online and approved stable 0.8.1 publication.
Windy now uses the verified public GTS Root R1 to avoid the older bundle verifier's
cross-sign failure. Stable 0.8.1 protected health reports HTTP code, negative TLS error code
(zero after success), last attempt and DNS information without exposing secrets.
See WINDY_TLS_FIX.md and certs/README.md. No 0.8.0 asset was replaced or republished.
