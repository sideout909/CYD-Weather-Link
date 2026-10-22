# Windy.com v2 — 0.8.0-rc.1

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
bounds, IDs and response codes. No real uploads or credentials have been used.

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
The uploader is included in the SC05_X build. Live Windy ingestion remains
unverified: the owner's device has no Windy credentials configured, and no test
observations were submitted to the real service. Register a station, enter its
password in setup and verify ingestion separately during owner acceptance.
