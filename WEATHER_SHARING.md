# Weather-sharing targets

Network research checked October 16, 2026; firmware status updated October 22 for stable 0.8.2.
APRS-IS/CWOP, Weather Underground and Weathercloud adapters are implemented.
The owner confirms APRS.fi, WU and Weathercloud ingestion; separate CWOP/MADIS
acceptance remains open. Windy.com v2 and PWSWeather are implemented; Windy
ingestion is owner-confirmed, while PWSWeather ingestion is now owner-confirmed. The PC logger remains a local development capture.
Recommendations reflect documented support and fit for this project; no
comparable current counts of amateur-radio users were established.

## Popularity evidence and implementation order

Use contributing station/device counts where published, not app downloads or
viewer counts. These are providers' claims with different definitions, not an
independent census of active stations or ham operators. No defensible complete
popularity ranking was established for all destinations.

| Proposed PWS implementation order | Published size evidence | Confidence and decision |
| --- | --- | --- |
| 1. Weather Underground | [About Our Data](https://www.wunderground.com/about/data) claims 400,000+ personal weather stations. Its [PWS overview](https://www.wunderground.com/pws/overview) still says 250,000+. | Strong evidence of a large network, inconsistent totals. Keep first. |
| 2. Weathercloud | [July 28, 2026 announcement](https://blog.weathercloud.net/announcements/2026/07/introducing-weathercloud-15-a-new-map-experience-with-progressive-zoom-smart-device-clusters-forecast-timelines-3d-cities-and-globe-view/) claims over 150,000 devices online. | Promote into the first group on published network-size evidence. Devices are not necessarily unique PWS owners. |
| 3. Windy.com Stations | No comparable current count of contributing personal stations found in the [station documentation](https://stations.windy.com/docs). | Provisional priority based on the separate weather map and documented upload API; not a verified third-place popularity claim. |
| 4. PWSWeather | No comparable current contributor count found on the [service site](https://www.pwsweather.com/). | Implemented opt-in; ingestion owner-confirmed. Relative popularity versus Windy is unresolved. |

Focus the first PWS upload work on Weather Underground and Weathercloud, with
Windy as the third destination. APRS/CWOP remains a core ham/public-service
requirement independently of this PWS ordering. Plan individually selectable
adapters for all four active PWS destinations; users can configure any supported
subset. This ordering is a roadmap; the first two PWS adapters are implemented. Met Office WOW is removed
as a new target because its retirement has been announced; WeatherWise remains
under investigation rather than being counted as a confirmed upload network.

## Recommended targets

| Destination | Benefit and relationship to ham radio | Project decision |
| --- | --- | --- |
| APRS RF and APRS-IS | Existing ham weather distribution and map/radio interoperability. | Direct APRS-IS implemented; owner confirms APRS.fi placement/weather. D710 serial remains optional/deferred. |
| CWOP / NOAA MADIS | Weather-observation contribution explicitly open to ham operators and other citizens; quality feedback and access for weather services. | Make supported CWOP participation an intended target, reusing APRS encoding and verifying routing/registration before adding any duplicate transmission. |
| Weather Underground | Existing requested personal-weather-station destination. | Implemented HTTPS uploader; owner confirms ingestion. |
| Weathercloud | Large published device network with maps, history, and a public developer upload API. | Implemented independently configurable uploader; owner confirms ingestion. |
| Windy.com Stations | Separate station map and observation-sharing API; useful wider weather audience, without a ham-specific identity requirement. | Implemented opt-in v2 uploader; owner confirms ingestion. TLS fix retained in 0.8.2. |
| PWSWeather | Separate PWS maps, history, and third-party integrations. | Implemented independent opt-in uploader; real-account ingestion owner-confirmed. |
| WeatherWise.app | Ryan Hall partner app; advertises viewing live weather stations. | Track possible integration; no advertised owner-upload path or public ingestion API was found. |
| Meshtastic environmental telemetry | Local mesh sensor sharing; usefulness depends on nearby users and mesh infrastructure. | Defer a bridge until a real local use case and compatible radio path are identified. |

Internet publishing targets use the existing ESP32 Wi-Fi and add no component
to the selected baseline. Meshtastic is a separate integration study; the
CC1101 Atlas receiver and D710 APRS serial path do not establish Meshtastic
compatibility. Preserve the single-controller appliance architecture and
bounded local history; SD packet logging is optional.

## CWOP: the first additional target

NOAA's [MADIS CWOP description](https://madis.ncep.noaa.gov/madis_cwop.shtml)
explicitly describes participation by amateur-radio operators and private
citizens, quality control, and public data availability. The
[current NWS participation page](https://www.weather.gov/fgz/cwop) invites
station owners to contribute for forecasting and warning operations.
This is the clearest additional destination for this project's ham/public
weather-service goal.

CWOP and APRS have related data formats and collection paths, so treat CWOP
as a distribution/registration requirement rather than assuming every station
needs a second simultaneous uploader. NOAA's
[registration form](https://madis.ncep.noaa.gov/cwop_signup.shtml) supports
registering an amateur callsign or obtaining a CWOP site ID. It also asks
contributors to disclose other internet publication because of duplicate
data concerns. The exact identity, server, authentication, and ingestion path
must be verified for the chosen station configuration.

As an implementation reference, the maintained
[pywws CWOP uploader documentation](https://pywws.readthedocs.io/en/latest/api/pywws.service.cwop.html)
also distinguishes callsign/passcode use from assigned citizen IDs. It is a
reference for profile behavior, not a dependency or a proposed second computer.

Implementation requirements:

- Share a validated APRS weather encoder and fresh observation store. Support
  explicit ham/APRS-IS and assigned-CWOP-ID connection profiles rather than
  treating identity, credentials, and server choice as interchangeable.
- First evaluate whether registration of the selected ham callsign/SSID and
  its existing APRS feed provides the required CWOP collection. Confirm the
  station actually appears in CWOP/MADIS before claiming success. A second
  publishing route is enabled only when the chosen configuration needs it.
- Keep endpoint/profile settings configurable and recheck official setup
  guidance at implementation. The WXQA server/FAQ pages could not be retrieved
  during this research; historical PDFs are not a verified current server list.
- The [NWS Cleveland guide](https://www.weather.gov/cle/CWOP) prefers five-minute
  updates, and the [APRS-IS operator guidance](https://www.aprs-is.net/) says
  weather beacons should be no faster than once per five minutes. Propose
  five-minute internet weather updates for the registered APRS/CWOP path,
  replacing the earlier ten-minute internet default. Set RF beacon timing
  separately for local channel use and coordinate any IGate duplication.
- Registration/metadata includes coordinates and site ground elevation. The
  entire Atlas being 30 ft above ground is mounting height, not site elevation.
  Retain both separately, along with exposure and pressure-sensor metadata.
- Verify temperature/humidity freshness, wind windows, rain reset/rollover,
  pressure type/reduction, clock validity, and missing-value encoding before
  activation. Review quality feedback after ingestion. Local collection and
  display do not depend on registration or an internet connection.

No account registration, submission, or contact with program staff was
performed as part of this research.

## Weathercloud: first-group PWS destination

The public [developer API](https://weathercloud.net/en/api) documents device WID
and key authentication, `/v01/set`, field scaling, UTC date/time, and an HTTPS
example. Use HTTPS and redact credential-bearing paths/query strings from all
diagnostics. The documented interval is ten minutes, or one minute for Pro and
Premium accounts; faster requests are rejected. Do not reuse another uploader's
cadence. An HTTP 200 alone does not establish success: the API documents its
actual result code in the response body, including 429 for excessive requests.

Map supported fields explicitly, including wind in m/s multiplied by ten and
daily rain in mm multiplied by ten. Confirm the averaging/peak windows, daily
rain boundary, pressure convention, assigned software-ID requirements, and
current registration process before implementation. Atlas lux is not the
API's solar-radiation measurement. Preserve lux locally without inventing W/m².

## Windy.com: optional separate upload

The [Windy station project](https://stations.windy.com/) accepts personal
station contributions for its map. Its
[current API reference](https://stations.windy.com/api-reference) specifies a
new API effective January 2026 and says the legacy API will cease operating
after the end of 2026. New firmware should target the current API rather than
copy older `/pws/update/...` examples.

The documented current observation path is
`https://stations.windy.com/api/v2/observation/update`. Upload/read authentication
uses the station password; station management and open-data access use API keys.
Follow the current schema and [station setup documentation](https://stations.windy.com/docs)
when implementing. Prefer the documented Bearer-header option for observation
authentication so a password need not appear in a logged URL. Do not log either
form of credential.

Registration, location, and credentials remain user configuration. Keep the
uploader disabled until configured; validate schema, units, wind/rain windows,
permitted update interval, and response semantics before enabling it. A local
TCP/HTTP send is not proof of map visibility; verify ingestion on Windy.
The setup documentation notes an initial map-registration delay, so confirm
activation before relying on the first uploads. This is observation sharing;
the NWS forecast page remains a separate feature.

## Other routes reviewed

- [PWSWeather](https://www.pwsweather.com/) advertises station maps, archived
  history, and integrations. Its [FAQ](https://www.pwsweather.com/frequently-asked-questions/)
  includes AcuRite setup and multi-service posting. Current detailed upload
  requirements still need verification before implementation; do not assume
  another service's endpoint, password, or field semantics can simply be reused.
- Met Office WOW was previously listed as a future candidate. The current
  [Met Office notice](https://weather.metoffice.gov.uk/learn-about/met-office-for-schools/other-content/other-resources/are-you-obsessed-with-the-weather)
  announces retirement later in 2026 and a move toward manufacturer data
  partnerships. Remove WOW from new development targets. This is an announced
  retirement, not a claim that the website is already closed. No equivalent
  direct replacement upload API was verified.
- [Meshtastic's telemetry documentation](https://meshtastic.org/docs/configuration/module/telemetry/)
  supports environmental data and lists temperature, humidity, pressure, light,
  wind, rain, and other sensor types. That supports investigating a local mesh
  bridge, not a claim that an Atlas decoded by this firmware can be uploaded
  automatically. A bridge needs compatible hardware/firmware, payload mapping,
  units, timestamps, and airtime/rate validation. No local mesh coverage is
  established, and no additional radio is added to `PARTS.md` now.
- [NWS SKYWARN](https://www.weather.gov/skywarn) is a spotter-reporting program.
  Treat a human's severe-weather report separately from continuous automatic
  station telemetry; no automated spotter-report submission is planned.

## WeatherWise.app: viewing is advertised; uploads remain unverified

The intended product is [WeatherWise.app](https://www.weatherwise.app/), the
Android/iOS radar app partnered with Ryan Hall, not unrelated products using
the same name. Its official site confirms the partnership and Y'all Mode.
The developer's [Android listing](https://play.google.com/store/apps/details?id=com.interactiveweather.weatherwise)
explicitly includes live weather stations alongside radar, alerts, satellite,
models, and storm-chaser feeds.

The reviewed official site and app listing did not advertise registering a
personal station for uploads, publishing observations from custom hardware,
or a public station-ingestion API. Search did not establish a documented route
from Weather Underground or CWOP into WeatherWise either. Viewing live stations
does not establish any of these upload capabilities. This is a scoped negative
finding, not proof that private or future integrations do not exist.

Keep WeatherWise on the project watch list because the owner uses it and values
the Ryan Hall community. Before promising integration, obtain official upload
documentation or confirmation of the station layer's upstream provider and
eligibility. No contact with WeatherWise was made during this research.

## Common exporter requirements

Keep one observation model and small destination-specific encoders/schedulers.
Each destination has its own enable setting, station identity, credentials,
supported fields/windows, interval, retry/backoff, and last-success/error state.
Only instantiate configured uploaders and bound TLS connections, queues, and
retry work so logging, radio reception, and serial output stay responsive.
Update connection/status details to identify the failing destination.

Do not forward every repeated RF packet to every internet service. Deduplicate,
compute requested measurement windows, and publish fresh observations at the
destination's allowed cadence. Preserve original observation timestamps and
field ages; stale cached data must not become a new observation on reconnect.
History backfill needs an explicitly supported destination format and policy.

Keep rain counters distinct from interval totals; measured station pressure
distinct from reduced pressure; lux distinct from solar irradiance in W/m².
Do not invent a universal lux-to-irradiance conversion to fill an upload field.
Retain raw readings and unsupported fields locally for later analysis.

Before activating any destination, record its current public requirements,
configure the owner's station identity/location, verify a sample encoded
observation without exposing credentials, and confirm actual remote ingestion.
Destination registration and provider-side field/routing checks remain
installation-specific acceptance work.

## Firmware implementation status — October 20, 2026

The initial [firmware](firmware/README.md) now includes opt-in Weather
Underground, Weathercloud and APRS-IS/CWOP transports, separate identities and
health states, fresh-live-only gating and retry/backoff. Software encoders
omit unavailable pressure, irradiance, sustained APRS wind and uncovered rain
windows. Defaults are off. Subsequent owner checks confirm APRS.fi map/weather placement and WU/
Weathercloud ingestion. Separate CWOP registration/collection, field-by-field
comparison and extended failure recovery remain acceptance work.

Windy.com v2 and PWSWeather were subsequently implemented as independent opt-in
adapters in 0.8.0. Windy ingestion is now owner-confirmed with the TLS fix retained
in 0.8.2; PWSWeather ingestion is owner-confirmed. Current code never sends replay
observations. Earlier research priorities above describe the original order.

## Owner-verified Weathercloud ingestion — October 21, 2026

The owner reports the configured device is connected to Weathercloud and the
service is receiving its data. Record Weathercloud alongside APRS.fi and WU
as owner-verified ingestion. This is an owner observation, not a new automated
comparison of every uploaded field or a validation of separate CWOP routing.
No account identifiers or upload keys belong in this record.


## 0.8.0 prerelease implementation

PWSWeather and Windy.com v2 adapters now join WU/Weathercloud/APRS-CWOP as
independent opt-in destinations. Both new integrations use five-minute successful
cadences and strict live field eligibility, with bounded retry backoff. They are
software-tested and built. Windy subsequently received HTTP 200 on the device and
the owner confirmed the station online; PWSWeather real-account ingestion is now owner-confirmed. Current stable 0.8.2 retains the Windy TLS fix and corrects OTA startup checks.
See firmware/PWSWEATHER.md and firmware/WINDY.md for authentication, units,
acceptance semantics and the current API contract. Display units never change
protocol units. APRS software/login and weather comment now identify WX-Link.
