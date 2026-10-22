# PWSWeather uploader

Included in stable 0.8.2 (introduced with 0.8.0). Live provider ingestion remains
unverified; software/build checks are recorded below.

Opt-in, independent of Weather Underground, Weathercloud and APRS. Configure a
PWSWeather station ID and station API key under Sharing. Blank key input keeps
the saved key; public configuration returns only the key-presence flag.

The implementation uses HTTPS GET to
`https://www.pwsweather.com/pwsupdate/pwsupdate.php`, the endpoint in the
[WeeWX uploader source](https://github.com/weewx/weewx/blob/master/src/weewx/restx.py).
Provider reference: [PWSWeather FAQ](https://www.pwsweather.com/frequently-asked-questions/).
This is PWSWeather, distinct from Weather Underground's similarly named PWS protocol.

Only live, fresh observations with a valid UTC timestamp less than 120 seconds
old are eligible. Missing, nonfinite and out-of-range fields are omitted; valid
zeroes are retained. Fields use Fahrenheit, mph, inches, percent and UV index,
regardless of the display's unit setting. Hourly and daily rain require valid
coverage. NWS pressure and inferred solar radiation are not submitted.

Successful requests are spaced five minutes apart. This is a conservative
project policy, not a verified provider minimum. Failures back off to at most
one hour. HTTP 200 alone is insufficient: recognized trimmed acknowledgments
are `success` and `Data Logged and posted in METAR mirror` (optional final period).
Unknown responses are reported as rejected. Response text, request URL and keys
are not exposed in status. `/api/status` includes `pws_state` and
`pws_last_success`; Connection health displays them.

Host tests cover protocol units, omissions/zeroes, encoding, stale/replay/future
rejection and acknowledgment checks. Browser tests cover enable/ID/key submission
and clearing the password input after save. Live provider acceptance, real station
credentials and retry timing on hardware remain unverified. No fabricated data
was submitted. Do not describe the integration as bench validated yet.
