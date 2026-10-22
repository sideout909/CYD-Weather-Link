# Home screen concept

Status: interface requirements and fictional study previews, updated through
local v0.7.3 on October 21, 2026. [Firmware scope](firmware/README.md) and
[validation](firmware/VALIDATION.md) distinguish installed behavior, owner checks
and remaining soak/service acceptance. Later owner changes below supersede
fictional study labels. Sound is removed from this hardware version.

This is page 1 of the implemented three-page swipe interface: **Home**, **Trends**
(rolling 24 hours), and **Forecast**. Navigation, shared layout, history, and
forecast behavior are specified in [SCREENS.md](SCREENS.md).

![Normal, watch, and warning home screen states](concepts/home-screen-v2.png)

The [original preview](concepts/home-screen.png) also illustrates loss of network service. The revised historical concept adds watch/warning themes; its earlier audio proposal is now removed.

The baseline is a **320×240 landscape touchscreen**, matching the owned PanelLan in [PARTS.md](PARTS.md). The preview uses that logical resolution, enlarged for review. Rendering/touch are bench observed; the illustration is not a live screenshot.

## Visual hierarchy

Use a dark navy background in normal operation, warm white numbers, cyan measurement graphics, and restrained green connection indicators. An active NWS watch changes the **entire screen background to orange**; an active NWS warning changes it to **red**. Cards, dividers, and secondary text use matching high-contrast variants. Every color has a text label or symbol. Avoid blinking, marquees, and decorative animations.

| Area | Content and interaction |
| --- | --- |
| Header, y=0–43 | Compact `WX LINK` title, larger centered callsign/SSID, configurable local date/time, Atlas health and received-packet age, and right-aligned RF/WiFi/APRS states with no sound icon. Subtle page dots indicate position. No large page-switching banner or permanent header arrows; swipes navigate. |
| Main readings, y=44–135 | Large outdoor temperature, a 60-minute trend, and a small NWS forecast graphic with local lux; current wind speed, compass arrow showing where wind comes **from**, and `Gust 5m` showing the highest received wind reading in the trailing five minutes. Tap either half for field ages, light/UV or wind/gust details. Swipe an outdoor/wind detail to open its trend. |
| Measurements, y=136–183 | Humidity, local-day rain, and BAROMETER with units/source. Tap a tile for details: humidity age/source, rain today/hour/24-hour totals and coverage, or pressure availability. Without a local barometer, fresh nearby NWS pressure can appear with NWS attribution; missing/expired data is unavailable. |
| Alert strip, y=184–239 | Highest-priority NWS event and alert count, or explicit check status. Tap anywhere in the strip for the alert list and full text. |

Use approximately 44-pixel or larger touch targets. Keep home reachable with a large Back button on detail pages. Detailed UV/light, lightning if fitted, battery and extended history belong on secondary pages; compact UV index/level accompanies klux on Home. NWS pressure is display-only; do not upload/archive it as local telemetry.

Horizontal swipes in the content area move between the three primary pages.
The alert strip remains a dedicated tap target on every page, and orange/red
themes apply across the interface. See [SCREENS.md](SCREENS.md) for gestures
and detail-page returns. The owner confirmed on October 20 that the main
screen should retain the v2 study's compact status header; the initial
`Home 1/3` banner and arrows were only a bring-up UI and have been removed.

## Graphics grounded in observations

The [weather icon sheet](concepts/weather-icons.png) contains twelve original icons: sunny, partly cloudy, cloudy, rain, heavy rain, thunderstorm, night, cloudy night, snow, fog, windy, and unknown. Editable SVGs and transparent 48×48 PNGs live in `assets/weather/`; [asset notes](assets/weather/README.md) define selection rules and export commands. They share the existing rounded cloud, cyan outline, and warm sunshine style. Rasterize to the display's actual icon size during firmware integration rather than assume a runtime SVG decoder.

Select graphics by source: calibrated lux supports labeled daytime sky estimates; recent rain-counter changes support rain; a defined recent rainfall rate supports heavy rain. A thunderstorm icon requires supported fresh local lightning evidence, or an explicitly labeled external condition source. A thunderstorm **watch or warning describes a hazard area**, not a confirmed storm at the sensor, and belongs in the alert presentation. Snow and fog assets are ready for a future supported source; do not select them solely from temperature, humidity, or darkness. Keep unknown available when evidence is insufficient. Configure precedence when rain/lightning overlaps a sky estimate, and expose the source on the detail page.

The large temperature, sparkline, compass, and estimated-sky graphic provide visual character without sacrificing readable values. Show a rain animation or `Rain detected` only from fresh rain-counter changes over a documented window.

The station has a lux sensor: expose its decoded reading and use it to support **estimated cloudiness**. Proposed approach: compare fresh lux against a calibrated clear-sky illuminance baseline at the station's solar elevation, with valid UTC, latitude/longitude, and a moving average to prevent flicker. Use coarse labels such as `Sky est. bright`, `Sky est. mixed`, and `Sky est. dim`, illustrated with sun/partial-cloud/cloud glyphs. These are inferred lighting conditions, not a measured cloud fraction. Do not assign numerical cloud-cover percentages from lux alone. Record sensor placement and shading; calibrate the baseline over verified clear days and validate labels against observations before enabling the estimate by default. No universal lux cutoffs are assumed.

The preview illustrates a possible calibrated estimate alongside `24 klx`; it does not demonstrate a working classifier. Until calibrated, show a neutral light glyph and lux. At night, low solar elevation, stale lux, invalid time/location, or sensor saturation, show the raw valid reading and `Sky est. —`. Lux alone cannot distinguish cloud from shade, fog, or precipitation reliably; retain that uncertainty on the detail page. A night glyph may indicate calculated nighttime without claiming a clear sky.

The owner reported roughly 15 klx in direct fall midday sun on October 16, 2026.
Retain this as an installation-specific reference, with higher summer readings
an expectation to test; do not hardcode 15 klx as the sunny/cloudy threshold.
Observation provenance is recorded in [CAPTURE.md](CAPTURE.md#reviewing-lux-rain-and-wind).

[AcuRite describes the sensor as measuring illumination at its installation location](https://kbase.myacurite.com/web/content/monitor/sensors/atlas-weather-station.htm?TocPath=Monitoring%7CYour+Dashboard%7C_____1). The cloudiness inference above is a proposed project feature, not an existing Atlas measurement or a validated algorithm. A Wi-Fi outage alone does not invalidate a locally computed estimate while its clock, coordinates, calibration, and lux remain valid.

The preview's sparkline and readings are illustrative, not a claim of measured history. In firmware, suppress a sparkline until enough valid samples exist.

The Home temperature sparkline spans the last 60 one-minute buckets. At the owner’s request, connect received points with straight lines across missed minutes instead of breaking the line.

This is display interpolation only: missing minutes remain missing in stored history, archives and exports, and cannot refresh measurements or qualify uploads. Draw an isolated observed point as a dot; do not extend the line beyond the first/last observation. Unchanged received values still populate buckets. The 24-hour Trends page independently draws each metric from its own populated buckets.

Display `Gust 5m` as the maximum valid wind-speed reading over the trailing
300 seconds, alongside current wind speed. This replaces the earlier proposed
ten-minute display window. Use wind from all applicable Atlas packet types,
deduplicate repeated copies without extending their original observation time,
and expire old peaks as time passes. Show the peak's time/age and history
coverage in the wind detail page. Mark startup/reception gaps as incomplete
coverage; apply wind freshness independently and show `—` once the window has
no valid wind samples. The full behavior and validation cases are specified in
[DESIGN.md](DESIGN.md#displayed-five-minute-wind-gust). Validate sensor sampling
limits before interpreting this maximum as a standard meteorological gust.

## Freshness and connection states

Packet reception and each measurement have independent freshness. A newly received packet cannot refresh fields absent from that packet. Station health shows `Disconnected` after five minutes without a bound-station packet. Mark each field `STALE` after ten minutes without its own update; also show `Data stale` after ten minutes with no change across accepted weather values. This unchanged-data warning does not prove a frozen transmitter. Missing values show `—`; cached values retain age and a visible stale label.

The header reports sensor reception and local Wi-Fi association independently; SD is optional and has no required header indicator. Status details show last RF reception, decoder/station ID, Wi-Fi/IP, time validity, history-checkpoint health, optional logger health when enabled, each configured uploader's last success/error (APRS-IS/CWOP route and individually selected PWS services from [WEATHER_SHARING.md](WEATHER_SHARING.md)), configured serial-output state, and separate NWS alert/forecast check health. CWOP registration/verified ingestion is distinct from local send success. Disabled outputs say `Off`. UART writes mean `Sending`, not confirmed radio receipt. Summarize any upload/storage fault in the header with an amber `!`; keep NWS events visible in their dedicated strip.

## Local NWS alerts

NWS supports active alerts for a configured latitude/longitude through its [point-filtered alerts endpoint](https://www.weather.gov/media/documentation/docs/NWS_Geolocation.pdf). Use station coordinates entered during setup, not IP geolocation or the developer machine's timezone. The location remains unconfigured until supplied.

```text
GET https://api.weather.gov/alerts/active?point={latitude},{longitude}&status=actual
Accept: application/geo+json
User-Agent: CYDWeatherLink/0.1 (project contact URL or email)
```

The [API documentation](https://www.weather.gov/documentation/services-web-api) currently describes free access and a required identifying User-Agent. Normal polling is every 30 seconds, honoring caching and retry headers; NWS [recommends no more than one request per 30 seconds](https://www.weather.gov/documentation/services-web-alerts). Refresh promptly after reconnect without bypassing throttling. Follow only expected HTTPS API redirects and verify TLS certificates.

Run this as a bounded, low-priority network task alongside existing uploaders. Stream/filter JSON to retain alert ID, references, event, headline, area, severity, urgency, certainty, status/message type, sent/effective/onset/expires/ends, description, and instruction. Follow pagination before treating a response as a complete snapshot. Cap retained alerts and text; explicitly display `More alerts` or `Details truncated` if limits are reached. A partial, oversized, or malformed response cannot establish an all-clear. Measure TLS/JSON memory with RF, display, history checkpointing, and all configured uploaders active.

Apply updates/cancellations using IDs and references, deduplicate versions, and exclude test/exercise messages from normal operation. Rank by CAP severity and urgency, with immediate tornado/severe-thunderstorm/flash-flood warnings at the top; retain unknown event types and let the list expose every returned alert. Watches, advisories, and unknown severity receive distinct text labels. Never infer an alert from local sensor values.

An alert detail page shows the official event, affected area, issue/update time, onset/end when supplied, issuing office, and scrollable NWS description and instructions. Use local timezone labels. Preserve the source text, wrap long headlines, and provide an alert-list button with count. Acknowledging an alert records that it was seen; it does not clear it. This hardware version uses visual alerts only.

## Full-screen alert themes

| Confirmed local NWS state | Screen background | Behavior |
| --- | --- | --- |
| No active alerts | Navy `#080F1B` | Normal measurements and last successful check. |
| Watch, with no warning | Orange `#944000` | Persistent `NWS WATCH` label and official event name. |
| Any warning | Red `#981F32` | Persistent `NWS WARNING` label; overrides concurrent watches. |
| Advisory or statement only | Navy | Labeled alert strip with amber accent; do not mislabel it a watch. |
| Unknown event classification | Navy | Show the official event and urgency; keep it in the list. |

These are project UI colors, not the official NWS map palette. Classify known NWS event names into warning/watch/advisory/statement, independently of CAP severity (which is not the product type). Use explicit mappings and a labeled unknown fallback. NWS [distinguishes watches, warnings, and advisories](https://www.weather.gov/sjt/WatchWarningAdvisoryExplained). Choose the overall theme from **all** applicable alerts; then rank events within the chosen category by urgency/severity for the headline. Keep all alerts accessible.

Apply the theme throughout home and detail pages. Change it on validated updates, without flashing. On a failed/stale check, retain the last known orange/red theme with a prominent `NWS UNVERIFIED` label and check age; do not quietly return to navy or imply confirmed ongoing activity. A successful complete snapshot can clear or downgrade the theme. Clearing/changing the configured location invalidates its cache and shows `Set alert location`/`Checking`. Silencing or acknowledging an alert never changes the background.

## Sound removed from current hardware

The owner removed sound alerts on October 21. No sound/mute indicator or driver
is included; [earlier plans](reference/FUTURE_AUDIO.md) apply only to a possible
future larger-display version. Visual watch/warning overrides remain.

## Alert strip states

- **Fresh complete empty result:** `NWS · No active alerts` plus last successful check age.
- **Active event:** event type, warning/watch/advisory label, count, and check age; tap to read. Do not automatically rotate away from the highest-priority event.
- **Check failed or older than three minutes:** `NWS unavailable` or `NWS data stale`, last successful check age, and a retained-alert badge when applicable. Never display an all-clear while offline.
- **Cached alert:** retain as `Last known / unverified` after a failed check or product expiry; do not continue labeling it confirmed active. `expires` describes message expiry and `ends` describes the event end; they are [not interchangeable](https://github.com/weather-gov/api/discussions/385). A successful complete snapshot resolves current membership.
- **Boot/time invalid/location missing/disabled:** explicitly show `Checking`, `Time unknown`, `Set alert location`, or `NWS off`, respectively.

When a new immediate severe/extreme alert arrives, wake a dimmed display and open its detail page once per new/materially changed alert; let the user return home while retaining the banner and theme. Also open details when an enabled audible alert starts, so Silence is immediately available. Outside those cases, preserve the current page. Use hysteresis for noisy connection failures. Internet-based alerts complement existing warning channels; local measurements and serial weather output continue independently during network loss.

## Acceptance checks

Review the preview at native size and on the real panel. Exercise long event names, multiple alerts, update/cancel, null end times, expired cached messages, empty results, pagination, malformed/oversized payloads, throttling, invalid time, reconnect, and a location change (which invalidates the old cache). Inject stale individual fields while RF remains active. Verify all text fits with Fahrenheit/Celsius and selected pressure units, and that alert fetching never stalls RF reception, history updates, or UART output.

Check normal/watch/warning transitions, simultaneous alert priority, advisories, unknown types and retained unverified themes offline. Verify contrast across palettes; orange stale readings and watch/warning colors retain their meaning.

## Owner updates — October 20, 2026

- Latest owner revision: main rain tile is **RAIN TODAY**, replacing the
  earlier rolling 24-hour request. Reset at midnight in the configured station
  timezone, including 23/25-hour daylight-saving days. With an existing rain
  baseline, the new day's display becomes `0.00` at midnight without waiting
  for another packet; coverage remains partial until reception confirms it.
  No baseline or valid date means `--`. `*` means partial/uncertain coverage;
  `!` marks stale rain-counter reception. Keep rolling hour/24-hour totals in
  rain details and for eligible service exports. Do not reset the sensor's
  lifetime/modulo counter. Rain spanning midnight has uncertain day allocation
  and prevents complete coverage for that day.
- Main weather graphics use the current NWS hourly forecast, explicitly labeled
  `NWS forecast`; they do not classify clouds from local lux. Hide graphics
  when Wi-Fi or fresh NWS hourly data is unavailable. Raw lux remains local.
- Download sun times and moon phase/illumination for the configured date and
  location. Use sunrise/sunset for day/night, with NWS `isDaytime` as fallback.
  Draw the phase when a moon is appropriate, respecting waxing/waning and
  hemisphere. If phase data is unavailable, do not invent a phase.
- Sun data attribution appears on setup and the portal: Sunrise-Sunset.org.

## Home v2 visual comparison — October 20, 2026

The firmware was compared directly with `concepts/home-screen-v2.png` and its
editable SVG at the native 320×240 screen size. It now packs DejaVu glyphs from
the concept's font family, including proportional spacing and antialiased
edges, instead of the initial thin bitmap fonts. Baselines, reading sizes,
tile positions, units, centered-dot separators, and normal/watch/warning
colors follow the study. Long temperature/wind values use a smaller number
font rather than overlapping the icon or running off-screen. Wind retains
one decimal place. Page dots occupy the gutter above the NWS strip.

The fictional study is still the visual reference, not a literal screenshot
of current behavior. These intentional text differences reflect later owner
requests or actual hardware/data state:

| Study text | Current firmware | Reason |
| --- | --- | --- |
| `RAIN TODAY` | `RAIN TODAY`, optionally `*` / `!` | Latest owner request restores local-midnight daily rain, with coverage/freshness. |
| `Gust 14 · 10 min` | `Gust … · 5 min`, optionally `*` | Five-minute received-sample maximum and incomplete coverage. |
| `Sky est. mixed` | `NWS` above the icon when available | Owner selected NWS conditions; local lux remains independent. |
| `Sound on` | Removed | Owner removed sound; center slot displays callsign. |
| `· SD` | Omitted | Owner removed required SD logging. |
| Fictional `30.06 inHg` | Nearby NWS pressure or unavailable | No local barometer; external data is labeled and display-only. |
| Fictional fresh Atlas values | Live Atlas readings or explicit stale/unavailable state | Live CC1101 reception is verified. |

The forecast icon itself follows actual current NWS conditions and downloaded
day/night/phase information; it need not match the study's fictional partly
cloudy icon. UTF-8 labels retain `°F` and centered dots. The setup page and
secondary screens continue using their existing fonts. Font sources/license
and generation instructions are recorded in the firmware README.

## Ham identity and Atlas transmitter health

Latest owner header request orders right-aligned RF, WiFi and APRS/CWOP statuses, with no outline around APRS/CWOP. The center slot
shows the configured callsign/SSID in
14-pixel bold text where space permits, falling back to the smaller header font for long identity/date formats.
Explicit USB replay still replaces that slot with REPLAY. RF + / RF X reports
receiver initialization; WiFi + / WiFi X reports association. APRS X / CWOP X
means disconnected or disabled, and APRS + / CWOP + means an acknowledged network
session (verified login is required for the ham profile). Network connection
and successful remote weather ingestion remain separate facts.

Tap the second header row for Link status: Atlas state, last bound-station
packet, missing types, APRS/CWOP session and last sent report. No receiver,
Waiting, Receiving, Current, Data stale and Disconnected are distinct states.
Show **Data stale** after ten minutes without any accepted weather-value change,
or after the ten-minute collection grace period if required types/measurements
are missing or ten minutes old. Any changed accepted weather field resets the
unchanged timer; rotating types, timestamps, burst duplicates and flagged
packets do not. Missing fields do not erase previously observed values.
Show **Disconnected** after five minutes without a bound-station packet,
including at startup; this takes precedence over the unchanged-data warning.
Bound-station flagged packets prove reception but cannot refresh measurements.
Other sensors and invalid packets cannot keep the station connected.

The owner requested these display thresholds after observing sunrise gaps.
Unchanged weather for ten minutes is a warning heuristic, not proof of a freeze.
Uploads retain the separate two-minute complete-cycle/field freshness gates and
are withheld while station health is stale or disconnected. Archive current-time
accounting also retains the two-minute completeness gate.



## October 21 source labels and pressure fallback

Owner request: remove the "NWS forecast" text beneath the local temperature;
that line now contains local klux and UV index/full level, with stale/missing markers when applicable. Keep
weather-icon attribution distinct from the local measured temperature.
Rename the pressure tile BARO. The current Atlas has no barometer, so display
fresh nearby NWS station pressure in inHg with NWS beneath the number. Prefer
sea-level pressure; otherwise use the station's reported barometric field and
name the type/source/observation age in pressure details. Do not silently claim
this is locally measured or include it in Atlas reports/uploads/archive.
Refresh observations approximately every ten minutes, rotate through up to three
nearby reporting stations on failures/missing pressure, respect retry delays,
and hide values older than 90 minutes or without Wi-Fi/valid time. A future
local barometer can take priority; no local pressure hardware is fitted yet.

### Main-screen stale temperature

The large temperature turns orange when the station is Data stale, when it is
Disconnected with a retained temperature, or when the temperature's own update
is at least ten minutes old. Remove the tiny STALE tag beside the number.
Missing temperatures remain muted dashes. Fresh data returns the number to
white. On Home, orange replaces the small Data stale status text; Disconnected
remains explicit. Other pages and measurement details retain the health text.
## Weather-icon source label placement — implemented v0.6.0

Owner request, October 21, 2026: move the **NWS** source label above the current
sunny/cloudy/night weather icon. Its current placement can make the nearby
temperature history graph appear to come from NWS. Keep the label visually
attached to the icon; the graph continues to represent local station readings.
This change is implemented; the local graph remains distinct from NWS icon attribution.


## Display preferences and shared themes — current implementation

Local v0.7.2 includes the later owner changes to the v2 reference: WX LINK,
larger centered callsign, configurable date prefix and 12/24-hour clock,
right-aligned RF/WiFi/APRS statuses, no sound icon and seconds since the last
decoded bound-station packet. Repeats prove reception independently of field
freshness. Receiving replaces the partial-cycle Collecting label.

NWS attribution sits above the weather icon. Local klux includes UV index and
full level (including Moderate); stale/missing UV is unavailable. BARO may use
fresh nearby NWS pressure, with source/type/age in details; it is display-only,
excluded from Atlas uploads/archive. Detailed trends preserve data gaps; only
the small Home sparkline bridges observed points across missed minutes.

Trends and Forecast return Home after 60 seconds without touch; setup, alerts
and measurement details are excluded. Keep-on/Dim/Off backlight preferences do
not sleep the MCU or pause RF, history, NWS or uploads. First-touch wake consumes
the gesture before normal controls resume. Touch/hold/release reset inactivity.

Station & Wi-Fi → Display & clock offers Midnight (original), Matrix, Amber,
Ice Blue, Violet, Paper and Night (low light). Browser changes preview locally;
Save and restart persists one palette for screen and webpage. Old settings
retain Midnight; invalid/nonstring themes are rejected. Shared JSON generates
RGB565 colors, embedded CSS and recolored original SVG weather artwork without
changing geometry. Watch/warning themes retain distinct colors; Paper uses a
darker readable stale orange. Sound is excluded from this hardware version;
[earlier audio plans](reference/FUTURE_AUDIO.md) are archived for larger displays.

Night uses near-black surfaces, subdued amber text/artwork/moon, and softer
olive NWS all-clear/connected indicators. Backlight is 24/255 normally, 2/255
when idle-dimmed and zero for Off; other themes remain 160/16. Wake returns to
the selected level. After display initialization, boot leaves the backlight
off until settings and the first themed frame are ready. Actual dark-room
appearance/wake checks remain owner validation.

Home uses current provider moon phase. Forecast icons estimate the phase at
each period midpoint from provider illumination/direction, anchored at noon of
the current local reference date and advanced with a mean synodic cycle.
This is a small-icon estimate, not an ephemeris. Invalid/missing current-day
lunar data remains unknown; hemisphere orientation, crater texture and cloud/
fog/precipitation overlays are preserved. No extra provider requests are made.

## Friendly settings address — v0.7.2

Advertise wxlink.local with mDNS and HTTP on TCP port 80. First-run and More
settings views show http://wxlink.local prominently, with current numeric AP/
LAN IP fallback. The browser header links both. Existing Basic authentication,
CSRF and private custom admin credentials are unchanged; no secrets are advertised.
Failed mDNS startup retries every 30 seconds and does not block core weather
operation. Existing setup DNS and 192.168.4.1 remain available. Default name
assumes one unit per local network; clients without local multicast support
can use the IP. No component, GPIO or storage-layout change is required.

## Original local palette studies

The [interactive concepts](concepts/themes/index.html), [comparison sheet](concepts/themes/comparison.png)
and [generation notes](concepts/themes/README.md) retain the original five
color-only studies. Their palettes were implemented in v0.7.0; the historical
SVGs retain their original sample header, including the since-removed speaker.

## Local v0.7.6 touch revision — pending device review

Owner markup replaces the old narrow Wi-Fi target with two full-height header
regions: x 0–103/y 0–43 opens Outdoor/status; x 104–319/y 0–43 opens the
on-device browser-settings guide. The Outdoor temperature target opens the same
combined page, now including receiver/APRS status. There is no separate Link
Status page. Daytime icons compose the sun and condition overlays separately,
as night icons already do with the moon. This build is local and unflashed.


## v0.8.0 prerelease additions

The header split and combined Outdoor/status behavior above are included. Units
are a persisted presentation setting, independent of raw history and sharing
protocol units. The pressure tile says BAROMETER. Its source remains NWS; details
identify AWC altimeter setting when used rather than claiming measured MSLP.

The original NWS-only sky requirement is superseded for the optional experimental
local daylight mode: a calibrated, fresh, stable lux estimate uses centered LUX attribution on the display (LUX* on the webpage).
Otherwise retain the existing NWS freshness/day/night/moon-phase rules. This does
not change forecast pages or alert sources. See firmware/LUX_CLOUDS.md for
calibration, low-sun exclusions and limitations; firmware/NWS_PRESSURE.md for the
investigated fallback. No inferred cloud data or external pressure enters uploads.


## Selected optional lightning concept — October 22

Owner chose B: alternate the Barometer tile with lightning every five seconds
while validated lightning activity/data changes are recent. Include last-strike
estimated distance and age plus the proposed ten-minute count. Stop rotating and
show only Barometer after thirty minutes without a real lightning-data change;
identical repeated packets do not refresh it. Stations without capability retain
normal Home. Keep last-event age separate from data-correction age; missing distance
is unknown, not zero. Miles are documented in the Atlas packet legend; reserved
codes and display agreement still need validation. Existing RF/field freshness
rules are independent from this Home activity expiry.

[Selected prototype](concepts/lightning/selected-v2/index.html) ·
[Both phases](concepts/lightning/selected-v2/proposal.png) ·
[Behavior and sources](concepts/lightning/selected-v2/README.md).
Local concept only; no firmware implementation or live lightning reception claimed.
