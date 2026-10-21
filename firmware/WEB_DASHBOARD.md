# Phone and browser dashboard

Version 0.5.0 expands the protected local setup page into a phone dashboard.
Join the station’s home Wi-Fi and open the local URL shown on the display.
Use the existing admin login. The setup access point also serves the page.

## Overview order

1. **At your station:** callsign; RF, Wi-Fi, APRS/CWOP and sound indicators;
   outdoor temperature; wind and bearing; five-minute gust; humidity;
   rain today; light and UV; NWS pressure; the NWS-based sky graphic; and
   a prominent local-alert strip. The small one-hour temperature line bridges
   missed minutes, matching Home. Retained stale temperatures turn orange.
2. **Trends:** select temperature, received wind peaks, light or humidity
   across the last 24 hours. Tap/point at the chart to inspect a minute.
   Missing minutes break the detailed graph; real zero values remain visible.
3. **Local forecast:** all retained NWS day/night periods (up to 14), full
   descriptions, wind, rain probabilities and issued time. Switch to the next
   24 hourly periods. Hourly cards scroll horizontally on phones; expandable
   descriptions keep the day/night list manageable.
4. **Weather alerts:** current or clearly marked last-known NWS headlines,
   descriptions, instructions and expiry. Unverified/partial/failed responses
   cannot establish an all-clear; retained warnings remain prominent.
5. Seven-day CSV recording/export and setup shortcuts, followed by sun/moon.

Temperature and gusts show one decimal place, rain two, and humidity/light
use appropriate whole-number units. Pressure uses two decimal places in inHg.
Sky graphics and pressure are attributed to NWS, separate from Atlas readings.
No forecast or pressure is invented when provider data is absent or expired.

A device without a saved home Wi-Fi network opens Station & Wi-Fi with a
prominent setup prompt. A saved network restores Overview as the landing page;
a missing location keeps a visible setup prompt for forecasts/alerts. Sharing
is optional and does not block the local dashboard.

## Data and refresh behavior

The dashboard uses the existing controller and cached NWS responses; opening
it does not cause provider requests. Status refreshes every five seconds,
minute history once per minute, cached forecasts every five minutes, and cached
alert details every 30 seconds while Overview is visible. Hidden tabs pause
these larger reads. Returning to the page refreshes due data. Fetches have
bounded timeouts and keep previous charts labeled unavailable if a read fails.

Protected read-only endpoints:

- `/api/status`: strict measurement fields plus a separate display-presence/age
  object, retaining the screen’s display behavior without weakening exporters.
- `/api/history`: at most 120 one-minute rows per request, with a pinned end
  minute and offset through the 1,440-entry ring. Rows contain UTC minute,
  temperature mean, wind peak, lux mean and humidity mean. Missing is JSON null.
- `/api/forecast`: four cached periods per page, daily or hourly, with fetched/
  issued times and explicit freshness. The browser rejects mixed revisions.
- `/api/alerts`: one cached alert per page with freshness, partial-response
  state and a revision marker. Failed reads retain an unverified status.

History reads run on the application loop against the existing ring; they do
not allocate another day of storage, write flash, replay samples or affect
measurement freshness. Display interpolation never enters exports or uploads.
All endpoints and local dashboard/icon assets require the existing admin
login. Weather/provider strings are rendered as text, never injected as HTML.
Credentials remain masked; no account secret is added to these responses.
The existing local HTTP transport is unchanged.

## Validation

Run `bash tools/test-firmware.sh` and the optional
`tools/verify_setup_page.py` / `tools/verify_location_setup.py` browser checks.
Host checks cover bounded history serialization, UTC paging, missing versus
zero, capacity exhaustion and read-only state. Browser checks use synthetic
packets/provider text only and verify formatting, graph gaps, day/hourly views,
warning prominence, safe text, stale/offline behavior, phone layout, setup-first
onboarding, settings ownership, masked secrets, CSRF and ZIP/map behavior.
Startup probes check dashboard authentication, its local gzip asset, forecast/
alert schemas, a full 120-row history page when UTC is valid, and invalid page
rejection without changing any saved settings.

### Owner-board validation, October 21

USB installation on the PanelLan SC05_X restored 638 populated minute buckets
and 134 archive intervals. All startup checks, including the new authenticated
endpoints, passed on the first attempt. Fourteen day/night and 24 hourly NWS
periods were available. Initial free heap was 141,456 bytes with 11,052 bytes
of minimum loop-stack headroom. Unauthenticated requests to history, forecast
and alerts each returned HTTP 401. Browser layout/onboarding/forecast/alert
checks and firmware host checks passed; live warning appearance and extended
phone/browser soak remain follow-up validation.
