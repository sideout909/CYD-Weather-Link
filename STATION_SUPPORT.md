# Weather-station integration priorities

October 22 follow-up: [Connect waveform review](firmware/CONNECT_WAVEFORM_REVIEW.md)
reran 16 public fixtures/40 decoded callbacks and checked all 20 decoder sources
against our pinned library. Iris/Notos are priority adapter candidates; production
support remains Atlas-only until model-bound integration and live acceptance.

Research date: October 21, 2026. This is a development shortlist, not a verified
sales or installed-base ranking. Atlas remains the only station family enabled
in CYD Weather Link. No receiver settings or firmware were changed by this research.

See [DECODER_SUPPORT.md](DECODER_SUPPORT.md) for the complete source inventory,
the reviewed 433 MHz CSV, actual emitted fields/unit mappings and the proposed
user-confirmed discovery contract.

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

### Inventory of the pinned ESP library

Direct source inspection confirmed the following decoder entry points at
`f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8`. This is a weather-focused inventory,
not a claim that every retail product under these brands is supported. Entries
can cover multiple models; several sensor decoders together may be needed for
one station. Frequency must be checked for each exact outdoor sensor/revision.

| Family / documented models | Decoder entry point(s) | Modulation | Scope / remaining gate |
| --- | --- | --- | --- |
| AcuRite Atlas, 5-in-1/Iris, 3-in-1/Notos | `acurite_txr` | OOK | Already registered; only Atlas accepted by application. Iris/Notos adapters and field-specific health required. |
| AcuRite 592TXR/592TX tower, 6045 lightning, 899 rain | `acurite_txr` | OOK | Same registered decoder, separate output models; no application integration yet. |
| AcuRite 896 rain, 609TXC, 606TX, 590TX, 00275/00276, 986 | `acurite_rain_896`, `acurite_th`, `acurite_606`, `acurite_590tx`, `acurite_00275rm`, `acurite_986` | OOK | Separate decoders for rain, temperature/humidity, probes and refrigerator sensors. Not complete wind/rain stations. |
| Sainlogic SA8 / Gevanti SA8 | `sainlogic_sa8` | OOK | Explicit model evidence; fix CRC rejection and validate rain units before use. |
| Fine Offset WH1050 | `fineoffset_wh1050` | OOK | Station adapter and exact RF validation required. |
| Fine Offset WH1080 / WH3080 | `fineoffset_wh1080`, `fineoffset_wh1080_fsk` | OOK or FSK variants | Determine actual variant; names do not select receiver mode automatically. |
| Fine Offset WH24 / WH65 / WS69, HP1000, MISOL WS2320; WH25/WH32 family | `fineoffset_WH25` | FSK | Includes station arrays and separate temperature/humidity/pressure products; verify RF band and model-specific fields. |
| Ecowitt / Fine Offset WS80, WS85, WS90 | `fineoffset_ws80`, `fineoffset_ws85`, `fineoffset_ws90` | FSK | Decoder source exists for each; band, rain capabilities and semantics differ. |
| Ecowitt WH53 / WH0280 / WH0281A | `ecowitt` | OOK | Outdoor temperature products; not all-in-one stations. |
| Ambient WH31E / Ecowitt WH40, WN20, WS68 | `ambientweather_wh31e` | FSK | Several sensor types share the decoder; verify regional band and associate sensors explicitly. |
| Oregon Scientific legacy weather sensors | `oregon_scientific` | OOK | Explicit IDs include THGR810, PCR800, WGR800, WGR968, UV800 and many others. Separate wind/rain/temperature sensors need user-confirmed grouping. |
| Oregon Scientific WMR500 | `oregon_scientific_wmr500` | FSK | Separate protocol from legacy Oregon devices; exact band validation required. |
| La Crosse WS-2310 / WS-3600 | `lacrossews` | OOK | Older station family; adapters and captures required. |
| La Crosse WS7000 / WS2500 | `lacrosse_ws7000` | OOK | Sensor family; association and field availability required. |
| La Crosse TX22U-IT | `lacrosse_tx22uit`, `lacrosse_tx22uit_ook` | FSK or OOK variants | Determine actual sensor variant and band. |
| La Crosse View LTV wind/rain/temp families, WS6868 | `lacrosse_breezepro`, `lacrosse_r1`, `lacrosse_th3`, `lacrosse_wr1`, `lacrosse_ws6868_tx232th`, `lacrosse_ws6868_tx231rw` | FSK | Source support does not establish 433 MHz; inspect exact hardware band. |
| Bresser 5/6/7-in-1; documented Froggit WH6000 / Ventus C8488A | `bresser_5in1`, `bresser_6in1`, `bresser_7in1` | FSK | Often another RF band; brand/model revisions require checking. |
| VEVOR YT60231 / YT60234 | `vevor_7in1` | FSK | Source explicitly documents 868 MHz EU and 915 MHz US. Requires appropriate band-specific receiver hardware/profile. |
| Holman WS5029 / AOK-5056 / Optex 990018 / BIOWIN 270208 | `holman_ws5029pcm`, `holman_ws5029pwm`, `holman_ws5029pwm_ook` | FSK and OOK variants | Multiple incompatible RF profiles under related branding; exact revision required. |
| Alecto WS3500 / WS4500, Ventus W155 / W044 | `alectov1` | OOK | Decoder lists these names; check sensor set and band. |
| Cotech 36-7959 / SwitchDocLabs FT020T; Cotech FT0203 | `cotech_36_7959`, `cotech_ft0203` | OOK | Weather-station and anemometer decoders respectively. |
| Hideki TS04; Thermor DG950; EMOS E6016 | `hideki_ts04`, `thermor`, `emos_e6016`, `emos_e6016_rain` | OOK | Separate sensors/rain decoder where applicable; adapters and association required. |
| Emax W6 / Newentor Q9 and documented rebrands | `emax` | FSK | Explicit model/rebrand list exists; does not establish support for Newentor Q3 or every Newentor product. |
| BALDR E0666TH, BALDR/RainPoint rain gauges | `baldr_therm`, `baldr_rain`, `rainpoint_hcs012arf` | OOK | Specific sensor support; not proof that every BALDR console kit uses these protocols. |

Sources: [pinned decoder directory](https://github.com/NorthernMan54/rtl_433_ESP/tree/f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8/src/rtl_433/devices),
[library receiver/decoder documentation](https://github.com/NorthernMan54/rtl_433_ESP/blob/f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8/README.md).

The library documents that OOK and FSK cannot be received simultaneously by
one receiver demodulator. Its default is OOK, with FSK marked experimental in
the header. A configured station profile can select an appropriate mode;
discovery across modes/bands would require measured time-sharing, not an
assumption of continuous reception on every profile. This does not require
changing the project's single-controller architecture.

Generic ingestion is feasible: normalize supported decoder output into a
common observation with model-specific capability, identity, cadence and rain
rules. Missing measurements must remain unavailable; unchanged valid readings
must not be discarded. Validate fields and integrity before updating health,
history or exporters. Decoder presence is the starting point for this work,
not a hardware-tested compatibility badge.

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

## Budget 433 MHz purchase and volunteer targets

Checked October 21, 2026. Prices below are advertised USD prices before tax,
shipping and batteries, not completed checkout quotes. Coupons and inventory
can change. This is a broad budget shortlist, not an exhaustive catalog of
changing marketplace brands. Manufacturer-direct prices are distinguished from
Amazon; Amazon pricing could not be reliably verified for the main candidates.
None of these findings enables another station in the firmware.

### Wind and rain stations, including sensor-only options

| Candidate | Advertised price / availability | Measurements and RF evidence | Development status |
| --- | --- | --- | --- |
| Ecowitt WS69 outdoor array only | $63.99; shipping extra; choose the **433 MHz** option explicitly | Temperature, humidity, wind speed/direction, rain, UV and light. Manufacturer offers separate 433/868/915 variants and specifies 16-second updates. | Strong sensor-only budget candidate. Existing Fine Offset decoder family uses FSK; receive-profile work and exact hardware validation remain required. Default North American selection is 915 MHz. |
| MISOL WS1080 complete station | $79.35 sale, $99.19 regular; seller page says in stock | Seller specifies 433 MHz, wind speed/direction, rain, temperature/humidity and console pressure; includes USB console. | Capture candidate. WH1080 OOK/FSK decoder variants exist, but the seller's WS1080 naming does not establish which protocol/revision ships. Verify delivered hardware and total shipping cost. |
| Sainlogic SA8 complete station | $80 advertised with `SA50OFF`; manufacturer currently sold out | Manufacturer manual: 433 MHz, 60-second outdoor updates; temperature/humidity, wind speed/direction and rain. No UV/light in this base model. | Existing SA8 decoder, with CRC rejection and rain-scaling blockers described above. Amazon listing ASIN `B0FFN4QJNQ` exists; no current Amazon price verified. |
| Sainlogic SA68 complete station | $84 advertised with `SA40OFF`; manufacturer page currently permits adding to cart | Manufacturer manual: 433 MHz, 60-second updates; temperature/humidity, wind speed/direction and rain. | Good budget capture target. Similar appearance and cadence do **not** establish compatibility with the SA8 decoder. |
| Sainlogic SA6 complete station | $108.99; manufacturer currently sold out | Manufacturer manual: 433 MHz, 60-second updates; temperature/humidity, wind speed/direction and rain. | Separate revision/protocol verification required. Do not infer support for SA6 Plus. |
| AcuRite Iris 5-in-1 outdoor array only, 06014MA2BK | $126.49 in manufacturer replacement-sensor catalog | 433 MHz family; temperature/humidity, wind speed/direction and rain; no UV/light. | Existing registered decoder offers a relatively short integration path. Confirm exact sensor revision, especially new 2026 units. |
| AcuRite Atlas outdoor array only, 06059M | $129.99 in manufacturer replacement-sensor catalog | Existing project's 433 MHz family; temperature/humidity, wind speed/direction, rain, UV and light. | Only family currently enabled and owner bench-verified. Catalog availability and individual product pages can differ; verify before ordering. |
| Raddy WF-100C complete station | $151.99 sale, $189.99 regular at Radioddity | Manufacturer/distributor specifies 433 MHz; wind, rain, temperature/humidity, UV and light. | Slightly above the roughly $150 budget ceiling; exact decoder association still unverified. Check destination/variant before purchase. |

Price/specification sources:

- [Ecowitt WS69, selectable frequency and sensor-only contents](https://shop.ecowitt.com/en-gb/products/ws69?variant=41418883465378).
- [MISOL WS1080 complete station](https://www.misolie.net/products/misol-professional-weather-station-pc-connection-wind-speed-wind-direction-rain-meter-pressure-temperature-humidity-with-solar-charge-functionoutdoor).
- [SA8 price/stock](https://www.sainlogic.com/products/sainlogic-wireless-weather-station-sa8),
  [SA8 RF manual](https://cdn.shopify.com/s/files/1/0912/1280/8488/files/SA8_User_Manual.pdf?v=1756797250),
  [Amazon SA8 listing](https://www.amazon.com/dp/B0FFN4QJNQ).
- [SA68 price/stock](https://www.sainlogic.com/products/sainlogic-wireless-weather-station-sa68),
  [SA68 RF manual, printed page 47](https://cdn.shopify.com/s/files/1/0912/1280/8488/files/SA68_User_Manual.pdf?v=1756797249).
- [SA6 price/stock](https://www.sainlogic.com/products/sainlogic-wireless-weather-station-sa6),
  [SA6 RF manual, printed page 47](https://cdn.shopify.com/s/files/1/0912/1280/8488/files/SA6_User_Manual.pdf?v=1756797249).
- [AcuRite replacement-sensor catalog](https://www.acurite.com/collections/replacement-sensors).
- [Raddy WF-100C price and 433 MHz specification](https://www.radioddity.com/products/raddy-wf100c-weather-station),
  [manufacturer WF-100C manual hosted by Amazon](https://m.media-amazon.com/images/I/C1jIRwWH0eL.pdf).

### Cheaper partial-weather options

- **AcuRite Notos 06008M: $54.99 sensor only.** Temperature, humidity and wind
  speed; no rain or wind direction. Existing `acurite_txr` decoder is promising,
  but the application still needs a partial-capability station profile.
  [Price catalog](https://www.acurite.com/collections/replacement-sensors),
  [Notos family RF/measurement specification](https://www.acurite.com/collections/notos/products/acurite-notos-3-in-1-weather-station-with-digital-display-2nd-gen).
- **BALDR WS0341BL4US: $19.99; WS0359BL5US: $34.99.** Manufacturer catalog
  currently marks these sold out. These are temperature/humidity-type systems,
  not wind-and-rain arrays. Manufacturer FAQ says its weather sensors use
  433 MHz; exact model/revision and decoder remain unverified. Keep them as
  low-cost capture leads rather than recommended supported purchases.
  [Catalog/prices](https://baldr.com/collections/weather-station),
  [manufacturer frequency FAQ](https://baldr.com/pages/weather-station-faq).
- **La Crosse WS9130: EUR32 sale on the French manufacturer site.** Explicit
  433 MHz, outdoor temperature only; a regional installed-base target rather
  than a complete APRS weather station.
  [Manufacturer price/specification](https://en.lacrossetechnology.fr/P-6-A1-WS9130.html).

### Marketplace leads that are not yet purchase recommendations

- **Sainlogic FT0835 / WS-0835 Plus:** documented 433 MHz manuals, but no
  reliable current US price obtained. Amazon FT0835 ASIN `B08P5VZKKJ` is a
  search lead; old deal prices must not be presented as live offers.
- **Gevanti SA8:** explicitly named by the existing SA8 decoder, but exact
  current listing, delivered revision and price still need checking.
- **Urageuxy:** Amazon ASIN `B0F8VGQ8D3` advertises wind/rain equipment, but
  frequency/protocol was not established from a matching primary manual.
  Do not transfer a 433 MHz claim from another Urageuxy model.
- **Newentor Q3 and generic Geevon/other inexpensive thermometer stations:**
  treat as model-specific research leads. A colorful forecast screen and
  a "weather station" title do not establish measured wind/rain or RF band.
- **MISOL MS-WH-5300:** inexpensive marketplace leads exist, but a matching
  manufacturer manual, dependable delivered price and decoder evidence were
  not verified in this pass. Do not merge it with WS1080 automatically.

Budget-looking alternatives can be on a different band: the
[Logia LOWSC510WB](https://logiaweatherstations.com/products/5-in-1-wireless-weather-station-with-wi-fi)
specifies 915 MHz US / 868 MHz EU-UK / 917 MHz AU. The
[Ambient WS-1965 manual](https://ambientweather.com/mwdownloads/download/link/id/1164)
specifies 915 MHz. These do not belong on a confirmed 433 MHz list.

For an eventual low-cost CYD bundle, evaluate the **WS69 array alone** and a
**Sainlogic SA8/SA68** sample. The standalone array could eliminate a redundant
vendor console once CYD compatibility is validated. Retain Iris as the first
engineering addition; budget research does not remove the Atlas validation gate
or change the single-controller hardware requirements.
