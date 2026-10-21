# Owner controls validation

Board: PanelLan ZX2D80CE02S-V10, 320×240 landscape. Record date/build and mark
each result. These checks use touch and a phone/browser; the CC1101 is not
needed for navigation. Missing local readings are expected until reception.

## Quick check for v0.5.1 automatic discovery

These are the priority owner checks; the remaining checklist can wait.

- [ ] Open the setup webpage on your phone, refresh it, and choose Station & Wi-Fi.
      Find my station is prominent; sensor ID entry is hidden under Advanced.
- [ ] Start detection. The five-minute progress indicator advances while the
      display continues receiving your currently configured Atlas.
- [ ] Your Atlas appears with sensible live readings. After at least three
      separate transmissions and a complete current measurement cycle, it asks
      “Is this your weather station?” and offers Use this station / Keep looking.
- [ ] Cancel once. Existing reception, history and sharing remain unchanged.
- [ ] Scan again, choose your Atlas, then Save settings. Confirm readings return
      after restart and the existing history remains available for the same ID.
- [ ] Swipe left and right through Home, Trends and Forecast and verify wraparound.
- [ ] On the phone Overview, check readings, Trends, forecast/alerts and download
      the seven-day CSV. Report any failed step; no need to test during bad weather.

Iris and Notos are detection previews in v0.5.1; they cannot yet be bound for
recording or uploads. Other library protocols are not enabled by this change.

## Display and navigation

- [ ] Home has the v2 compact header, no page-switching banner, and RAIN TODAY.
- [ ] If configured, your callsign appears in the header center. The speaker
      icon beside Wi-Fi is muted while optional audio is not fitted.
- [ ] APRS X / CWOP X means disconnected or off; APRS + / CWOP + means an
      acknowledged session. RF and Wi-Fi also have a space before + or X.
      Link details distinguish disabled from connection failures.
- [ ] Tap the RF/source or APRS part of the status row for Link status;
      the padded Wi-Fi area opens setup. Back returns Home. Verify Atlas
      status, last packet, missing packet types and APRS report age are clear.
- [ ] Swipe left: Home → Trends → Forecast → Home.
- [ ] Swipe right: Home → Forecast → Trends → Home.
- [ ] A swipe does not also open a tile, setup, or an alert.
- [ ] Tap the Home temperature half: Outdoor details show temperature age,
      light/lux age and UV age. Tap its Back header to return Home.
- [ ] Tap the Home wind half: Wind details show current speed, direction,
      five-minute peak, peak age, wind age and coverage. Back returns Home.
- [ ] Tap Humidity: percentage, age, source and sensor binding are visible.
- [ ] Tap Rain: Today, Last hour and Last 24 hours are separate; reset says
      00:00 local. Missing readings show unavailable, not invented rain.
- [ ] Tap Baro: Pressure identifies the nearby NWS station and observation age
      when available, or says unavailable. Back returns Home.
- [ ] Swipe horizontally inside Outdoor/Wind/Humidity details: the matching
      temperature/wind/humidity trend opens. Tap the chart to cycle metrics.
- [ ] Home still shows the downloaded NWS icon/phase when eligible; local
      missing or stale readings remain independently labeled.

## Forecast and alerts

- [ ] On Forecast, tap Daily/Hourly to switch products.
- [ ] Tap forecast body to advance periods; drag vertically to scroll text.
- [ ] Vertical dragging does not switch primary pages or advance a period.
- [ ] Tap the NWS strip from Home, Trends, Forecast and a measurement detail.
      Alert text opens; tap its header to return to the original view.
- [ ] With no alerts, details say No active alerts; check age advances and
      refreshes. Cached/unverified data must not claim a fresh all-clear.
- [ ] When multiple real alerts exist, bottom tap advances alerts and dragging
      scrolls text. Do not wait for dangerous weather just to test this.
- [ ] If an actual watch/warning occurs, the orange/red theme persists across
      pages and measurement details. Earlier labeled previews verified colors;
      acknowledging/returning must not clear the NWS event.

## Setup and location

- [ ] Tap the title or Wi-Fi indicator: setup network/Wi-Fi key and LAN address
      appear; the full 44-pixel header closes it. No sun-data footer appears here.
      Keep that access key private.
- [ ] On a phone, open the displayed URL and sign in as admin using your saved admin password (initially the Wi-Fi key).
- [ ] With a custom admin password, setup shows only the generated Wi-Fi key
      and “custom password set”; your chosen password never appears. Verify
      changing the admin password leaves the setup Wi-Fi key unchanged.
- [ ] Overview, Station & Wi-Fi, Sharing, Wiring & health and Updates tabs all work.
- [ ] Network scan populates choices; blank password/key fields retain saved
      secrets. Avoid saving unrelated changes solely for this check.
- [ ] Manual coordinates/elevation remain editable with no map/internet.
- [ ] ZIP search centers the embedded map without changing saved form values.
- [ ] Pan/zoom the map, drop a pin and drag it to the station. Its displayed
      coordinates and ground-elevation estimate update to the selected point.
- [ ] Use this location copies the confirmed point/elevation into the form.
      The location checkbox becomes selected; sharing toggles/identity stay as set.
- [ ] Save and restart only after reviewing the values. Confirm the saved
      location, elevation and timezone survive restart and NWS recovers.
- [ ] A bad ZIP or failed map/elevation request gives a useful message; manual
      entry still works. Failed elevation must not reuse the previous site's value.

## After the radio is connected

- [ ] Follow [WIRING.md](WIRING.md), including the two debug-header power pins.
- [ ] RF + appears, packet count increases and values arrive without replay.
- [ ] Repeat the swipes/details while fresh packets arrive; ages stay sensible.
- [ ] After five continuous minutes, gust coverage becomes complete.
- [ ] Rain TODAY is partial on initial startup. It resets at local midnight;
      rolling totals in details retain prior rain until their windows expire.
- [ ] A reception outage produces stale fields/partial coverage rather than
      refreshing old data. Reconnect reception and check recovery.
- [ ] If required data is missing or unchanged for ten minutes, the main
      temperature turns orange. After five minutes without a bound packet,
      status instead says Disconnected. Neither condition refreshes old fields.

## Optional firmware updates

- [ ] The Updates tab accepts a public owner/repository; check/automatic
      installation are off by default. Leave them off until releases exist.
- [ ] Check now reports installed/latest versions or a clear lookup error.
- [ ] A compatible newer release permits Install update and restart. Record
      versions, verify settings/history recovery, then confirm radio/network
      operation. Test interrupted downloads/rollback separately on the bench
      before enabling unattended installations.

Record failed action, expected/actual result, page and approximate time.
Full reception, outage, persistence and sharing gates are in [VALIDATION.md](VALIDATION.md).


## Seven-day recording

- [ ] Overview shows recording ready, saved interval count and free storage.
- [ ] After a UTC five-minute boundary, the saved interval count increases.
- [ ] Download seven-day CSV succeeds and opens with UTC timestamps, real zeros,
      blank missing measurements, received wind peaks and explicit coverage flags.
- [ ] Normal RF readings/swipes remain responsive during the download.
- [ ] Move to independent USB power; Wi-Fi/Atlas recover, earlier archived rows
      remain available and any restart gap stays partial. Allow up to five
      minutes of uncommitted data loss on an abrupt disconnect.
- [ ] Bring the board back after the soak and export the rolling week via USB
      or browser. Keep that CSV separately to preserve it beyond seven days.
