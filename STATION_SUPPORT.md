# Weather-station integration priorities

Research date: October 21, 2026. This is a development shortlist, not a verified
sales or installed-base ranking. Atlas remains the only station family enabled
in CYD Weather Link. No receiver settings or firmware were changed by this research.

## What popularity evidence can establish

Current manufacturer catalogs, regional RF specifications, active decoder source
and first-hand product tests identify plausible integration targets. They do
not establish comparable worldwide unit counts. Keep current purchase interest
separate from the installed base of older stations, and separate both from
engineering priority. Review counts, editorial picks and a manufacturer's
“best-selling” label are supporting signals, not market share.

AcuRite currently sells a 433 MHz Iris and advertises a sensor revision new for
2026. Sainlogic currently markets SA8 and related stations; Consumer Reports
also includes FT0835 in its tested range. This supports investigating those
families, without proving an Iris-versus-Sainlogic sales order.
[Current Iris](https://www.acurite.com/collections/gifts-over-100/products/acurite-irisr-5-in-1-weather-station-with-direct-to-wi-fi-display-and-solar-power-pack),
[SA8](https://www.sainlogic.com/products/sainlogic-wireless-weather-station-sa8),
[FT0835 product test](https://www.consumerreports.org/home-garden/weather-instruments-sensors/sainlogic-8-in-1-weather-station-ft0835/m416064/).

## Weather Underground as an installed-base proxy

The public [PWS API index](https://www.wunderground.com/pws-api) documents
observations, history, location search and forecasts. No current public
make/model-count endpoint or hardware-share report was found in that review.
The [buying guide](https://www.wunderground.com/pws/buying-guide) is a compatibility
list containing older products, rather than a current usage ranking.

The [current-observation schema](https://www.wunderground.com/pws-api/observations/current-conditions)
includes nullable `softwareType`. It may resemble a console model, but can also
identify uploader software; the documentation's example uses GoWunder. It is
not a guaranteed outdoor-sensor identity. `realtimeFrequency` means reporting
interval in minutes, not RF frequency. Neither field establishes 433 MHz.
Observations and location queries require a read API key; the upload station
key is a different credential. The documented query requires `apiKey`, and
[WU's support instructions](https://support.weather.com/s/article/How-to-Get-a-Copy-of-Your-Personal-Weather-Station-PWS-Data)
describe creating that key through account settings.

A future authorized, bounded sample could tally uploader strings by region,
retain unknown hardware, and distinguish recently active stations. Hardware
and RF band would need separate verification. Such a sample would represent
WU-connected stations, excluding offline-only consoles, and would need a
sampling design before it could support wider installed-base estimates.
No such census was performed here; no percentages or model counts are claimed.

## Recommended engineering order

| Priority | Family | RF and decoder evidence | Integration implication |
| --- | --- | --- | --- |
| 1 | AcuRite Iris / legacy 5-in-1 | Manufacturer specifies 433 MHz. Our existing `acurite_txr` decoder emits `Acurite-5n1` using OOK. | Best first full-station addition: reuse the registered decoder, add a separate profile/adapter and reviewed captures. Verify the revised 2026 sensor independently. |
| Early bonus | AcuRite Notos / 3-in-1 | Manufacturer specifies 433 MHz; `acurite_txr` also emits `Acurite-3n1`. | Low decoder cost, but temperature/humidity/wind only. Missing rain, wind direction, light and UV must stay unavailable rather than prevent all updates. |
| 2 | Sainlogic SA8 / documented Gevanti SA8 rebrand | Manufacturer SA8 manual specifies 433 MHz and 60-second updates; pinned `sainlogic_sa8` uses 433.92 MHz OOK/PCM. | Promising current family, but integrity handling and rain scaling require validation before enabling. Do not assume other Sainlogic model names share it. |
| 3 | Fine Offset / Ecowitt WH65 and WS69, verified 433 MHz variants | Pinned `fineoffset_WH25` handles WH24/WH65/WS69 using FSK. Its source documents regional 433/868/915 MHz variants. | Broad family potential after a tested FSK receive profile. A label such as `Fineoffset-WH65B` alone does not establish the received RF band. |
| Later | Older Oregon Scientific WMR-family and La Crosse WS-23xx/36xx systems | Pinned Oregon and `lacrossews` decoders exist, with OOK support for specified older equipment. | Installed-base preservation targets; no evidence here establishes them as leading current sellers. Oregon's separate wind/rain/thermometer sensors also need an explicit association design. |

Sources: [Iris RF specification](https://www.acurite.com/collections/gifts-over-100/products/acurite-irisr-5-in-1-weather-station-with-direct-to-wi-fi-display-and-solar-power-pack),
[Notos RF/measurements](https://www.acurite.com/collections/notos/products/acurite-notos-3-in-1-weather-station-with-digital-display-2nd-gen),
[manufacturer-hosted SA8 manual](https://cdn.shopify.com/s/files/1/0912/1280/8488/files/SA8_User_Manual.pdf?v=1756797250),
[SA8 decoder](https://github.com/NorthernMan54/rtl_433_ESP/blob/f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8/src/rtl_433/devices/sainlogic_sa8.c),
[AcuRite decoder](https://github.com/NorthernMan54/rtl_433_ESP/blob/f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8/src/rtl_433/devices/acurite.c),
[Fine Offset decoder](https://github.com/NorthernMan54/rtl_433_ESP/blob/f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8/src/rtl_433/devices/fineoffset.c).

### Families that need model-specific research

Sainlogic FT0835/WS-0835 Plus is a separate capture candidate. The
[manufacturer's WS-0835 Plus manual](https://www.sainlogic.com/media/hexaattachment/products/attachments/ft0835plus-wifi_manual_usa.pdf)
specifies 433 MHz, but that does not prove the SA8 decoder applies to it.
SA68, SA9, Plus, and other revisions require their own protocol evidence.
Do not infer OEM identity from a similar-looking array.

Older WH1080/WH3080 systems have both OOK and FSK decoder variants in the pinned
library. They can be useful volunteer targets but do not establish support for
all newer Fine Offset or Froggit products.

## Popular brands do not imply 433 MHz

Ambient calls WS-2902 its best-selling station, but specifies **915 MHz** for
sensor-to-console RF. Its 2.4 GHz Wi-Fi is a separate link.
[Manufacturer specification](https://ambientweather.com/ws-2902-smart-weather-station).

Other examples:

- VEVOR YT60234 specifies 915 MHz for US and 868 MHz for EU/AU/UK. Its broad
  category text mentions 433 MHz elsewhere; the actual model specification
  and matching decoder take precedence.
  [YT60234 specification](https://www.vevor.com/weather-station-c_12074/vevor-7-in-1-wi-fi-weather-station-7-5-in-color-display-home-weather-station-indoor-outdoor-with-solar-wireless-outdoor-sensor-alarm-alerts-for-temperature-humidity-wind-speed-direction-rain-p_010918829979).
- Bresser 7002510 and the reviewed Froggit WH3000 product specify 868 MHz.
  [Bresser](https://www.bresser.com/pl/p/centrum-pogodowe-bresser-5-w-1-czarne-7002510),
  [Froggit](https://www.froggit.de/Froggit-WH3000-WiFi-Internet-Wetterstation.html?language=de).
- La Crosse says its sensor frequencies span 433–915 MHz and directs owners to
  model-specific manuals. A brand-wide compatibility claim would be misleading.
  [Manufacturer guidance](https://www.lacrossetechnology.com/pages/wireless-transmitting).

Reaching these audiences would require appropriate band-specific hardware and
validated receive profiles. The present 433 MHz module/antenna is not evidence
of reliable reception on another band. No component requirement has been
changed; the single-controller architecture remains in place.

## Decoder and model work required before release

The installed library is pinned at
`f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8`, inspected without local modifications.
The build registers only `acurite_txr`; the application then accepts Atlas only.
A decoder being present in the library is not a CYD compatibility claim.

- Define profiles for available fields, cadence, units, required completeness,
  identity and rain-counter behavior. Atlas message types 37/38/39, 10-bit ID
  limit and 9-bit rain-counter handling cannot be universal. The older 5-in-1
  decoder extracts a 12-bit ID and a wider rain counter.
- Preserve both sensor-reported gusts and their meaning. SA8 emits a gust as
  well as average wind; our maximum received sample over five minutes is a
  different statistic. Verify packet counters and reset behavior on real units.
- Resolve the SA8 integrity gate first: the pinned decoder logs a CRC mismatch
  but continues toward emission. Do not treat its `mic: CRC` label as proof
  that a bad frame was rejected. Require negative-CRC fixtures and a reviewed
  fix before allowing live uploads from this family. Its rain conversion also
  needs measured validation against the manual/console.
- Treat FSK as a separate receive-profile milestone. The pinned library defaults
  to OOK and labels its FSK mode experimental. Adding an FSK decoder name does
  not change the receiver into a validated FSK configuration. Mixed-mode
  scanning/time-sharing can miss packets and needs measurement.
- Keep user-confirmed discovery plus manual selection. Group candidates by
  protocol/model, full sensor identity, optional channel and RF profile, using
  independent valid transmissions. A short scan can miss a 60-second sensor.
  Never silently bind to the first/strongest neighboring device.
- Recruit by exact outdoor model, regional frequency and hardware revision.
  Review complete-cycle packet captures, rain examples, reset/wrap cases and
  missing/stale behavior before publishing fixtures. The existing PC logger
  is Atlas-bound, so a generic capture workflow remains a separate TODO.

Keep decoder-present, fixture-tested, RF-verified, and soak-validated labels
separate. Prioritize Iris capture volunteers first, then a verified SA8/Gevanti
unit, followed by 433 MHz Fine Offset FSK volunteers.
