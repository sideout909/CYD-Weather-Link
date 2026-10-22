# Decoder inventory, 433 MHz scope and station discovery

October 22 follow-up: [Connect waveform review](firmware/CONNECT_WAVEFORM_REVIEW.md)
reran 16 public fixtures/40 decoded callbacks and checked all 20 decoder sources
against our pinned library. Iris/Notos are priority adapter candidates; production
support remains Atlas-only until model-bound integration and live acceptance.

Source review: October 21, 2026. Library: `rtl_433_ESP` pinned at
`f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8`. These are library capabilities and
proposed integration rules. CYD Weather Link currently accepts **Atlas only**.
The source inventory itself does not enable protocols. The subsequent v0.5.1
implementation below adds Atlas discovery and Iris/Notos detection previews.

## Implemented discovery in v0.5.1

The setup page now offers Find my station, with a five-minute scan, three
independent sightings, per-field freshness, candidate readings and explicit
selection followed by Save settings. It uses the existing `acurite_txr`
433.92 MHz OOK profile. Iris/Notos detections are labeled integration pending
and cannot be selected. FSK and the remaining inventory are future work.

Native radio callbacks alone enter discovery; USB replay is excluded. The
confirmed existing binding continues normal reception during a passive scan.
Unbound/foreign discovery samples never enter the operational weather store,
archive or upload queue. A new device remains unbound until confirmation or
explicit Advanced manual binding. Existing saved station IDs migrate as
confirmed; no credential changes or additional receiver modes are involved.

Candidate storage is bounded to 16 entries. Selection uses the current random
scan session and a non-reused candidate token; the server rechecks three
sightings, fresh complete Atlas fields, eligibility and CSRF at settings save.
Expired/cancelled/replaced detections fail without changing saved settings.
An expired scan's candidate can still be selected only while its required
fields are individually current (less than two minutes old). Cancelling
invalidates all selections. Starting/cancelling is authenticated and CSRF protected.

Files: `station_discovery.h` (bounded host-testable model), `discovery_packet.h`
(native model/unit mapping), `services.cpp` (protected API/persistence),
`assets/web/discovery.js` (local setup UI). Atlas types 5/6/7 normalize to
37/38/39 so stations without the lightning extension use the existing profile.
Full library availability is still not operational support for every model.

## Device lists

- [All decoder declarations, CSV](reference/decoder-inventory/all-decoders.csv):
  375 protocol entries, including non-weather devices.
- [Weather/environment candidates, CSV](reference/decoder-inventory/weather-candidates.csv):
  230 entries selected by names and declared fields. This intentionally broad
  heuristic includes unrelated products such as thermostats, probes and TPMS
  reporting temperature. It is a review list, not a count of complete weather stations.
- [Reviewed 433 MHz weather subset, CSV](reference/decoder-inventory/433-weather.csv):
  27 decoder entries; some explicitly require the 433 MHz regional variant.
- [Complete machine-readable inventory](reference/decoder-inventory/all-decoders.json):
  symbols, decoder names, modulation, disable flags, declared field lists,
  source links and file-level frequency mentions.

One decoder may recognize several products. A multi-sensor station may require
several decoders. Counts are neither retail device counts nor enabled firmware
protocol counts. Preprocessor conditions are not evaluated by the inventory.
An upstream `.disabled = 1` is preserved, not silently overridden.

Regenerate with `python tools/inventory-decoders.py` after dependencies are
installed. The 433 MHz subset uses reviewed model scopes in that script, not a
search for the number 433. File-wide frequency comments can refer to a different
model sharing the same source file. Review the subset when changing the pin.

The declared `.fields` array is an optional union used by upstream output
formatting. It is **not** a guarantee of the actual JSON keys in every callback.
Read each `data_make` / `data_append` path for integration. In particular, SA8's
declared wind fields differ from its actual emitted fields in this revision.

## Reviewed 433 MHz candidates

| Device/protocol scope | Receiver mode | Evidence and boundary |
| --- | --- | --- |
| AcuRite Atlas, Iris/5-in-1, Notos/3-in-1 | OOK | Manufacturer 433 MHz specifications plus `acurite_txr` source. New revisions still need captures. Other models in this decoder are not automatically included. |
| Sainlogic SA8 / documented Gevanti SA8 | OOK | `sainlogic_sa8.c` explicitly says 433.92 MHz. Integrity fix required. |
| Fine Offset WH24/WH65/WS69; MISOL WS2320 | FSK | `fineoffset.c` documents 433/868/915 variants and MISOL WS2320 at 433. Only verified 433 hardware belongs here. |
| Fine Offset WH1080/WH3080 | OOK or FSK variant | `fineoffset_wh1080.c` documents 433/868.3/915 versions; select the matching decoder and actual variant. |
| Cotech 36-7959; SwitchDocLabs FT020T; Sainlogic WS019T/FT0300; Ragova FT-0310; NicetyMeter 0366 | OOK | `cotech_36_7959.c` explicitly documents 433 MHz and these rebrands. Some lack light/UV. Do not generalize to FT0835 or similarly named products. |
| La Crosse WS-2310/WS-3600; legacy TX family | OOK | `lacrossews.c` and `lacrosse.c` explicitly document 433 MHz; not a claim about all La Crosse products. |
| EMOS E6016/E6018 and E6016 rain sensor | OOK | Corresponding source files explicitly specify 433.92 MHz. |
| Telldus FT0385R sensors | OOK | Source explicitly specifies 433 MHz. |
| Ambient F007TH / related TFA/SwitchDocLabs thermo-hygrometers | OOK | Source documents 433/868 MHz variants; verify the exact sensor. |
| Auriol AFW2A1; Esperanza EWS-103; Globaltronics GT-WT-02/03 | OOK | Each corresponding source file explicitly identifies the 433 MHz band. |
| Cotech 36-7900; RainPoint HCS012ARF; Schou 72543 and documented rain-thermometer rebrands | OOK | Corresponding source files explicitly identify 433 MHz. |
| EMOS TTX201; Vauno EN8822C; Hyundai WS SENZOR | OOK | Corresponding source files specify 433.92 MHz. |
| Kedsum / Pearl NC-7415 temperature/humidity | OOK | Source reports approximately **433.71 MHz** on tested units. A single center-frequency choice is not universal. |
| Homelead HG9901 soil/temp/light; RainPoint soil/temp | OOK | Source documents EU 433.92 MHz for Homelead and 433.9 MHz for RainPoint. |
| TBH weather sensor; Inkbird ITH-20R | FSK | Source specifies 433.93 and 433.92 MHz respectively. |

Sources are linked per row in the CSV. For AcuRite, see the
[current Iris RF specification](https://www.acurite.com/collections/gifts-over-100/products/acurite-irisr-5-in-1-weather-station-with-direct-to-wi-fi-display-and-solar-power-pack)
and [Notos RF specification](https://www.acurite.com/collections/notos/products/acurite-notos-3-in-1-weather-station-with-digital-display-2nd-gen).
Atlas is also the existing project's hardware-verified 433.92 MHz receiver case.

### Other-band and unresolved candidates

- VEVOR YT60231/YT60234 source explicitly identifies 868 MHz EU / 915 MHz US.
  Bresser 5-in-1 source identifies 868.3 MHz. Keep these out of the confirmed
  433 list despite existing decoders.
- Ecowitt WS80/85/90, other Bresser/La Crosse families and Oregon Scientific
  have source decoders, but model/revision-specific band evidence was not fully
  established in this review. They remain in the full inventory, not silently
  classified as incompatible or 433 MHz.
- SA6/SA68 manuals specify 433 MHz, but a matching decoder association is
  unverified. Do not label them SA8-compatible merely from shape or cadence.
- BALDR E0666TH has a decoder, but its exact hardware RF evidence needs review.
  `baldr_rain` is explicitly disabled upstream because it lacks validity and
  checksum checks. A brand-wide FAQ is insufficient to enable every model.

## What arrives at our application

The present path is: CC1101 demodulator -> captured pulse timings -> rtl_433
bit slicing -> protocol decoder -> typed data fields -> JSON callback -> bounded
application queue -> Atlas-only adapter -> weather store/history/exporters.
Source: `firmware/src/radio_receiver.cpp`, `firmware/include/atlas_devices.h`
and `firmware/include/atlas_packet.h`.

The decoder already converts raw bitfields to engineering units. We should
normalize those emitted values, not decode the RF payload a second time.
The JSON callback does not inherently establish exact retail hardware identity,
station ownership, RF band, or that every desired measurement was present.
Record the receiver profile alongside candidates. Invalid or omitted fields
must stay unavailable rather than become zero or inherit a fresh timestamp.

### AcuRite Iris / 5-in-1

Source: `acurite.c`, `acurite_5n1_decode`, caller `acurite_txr_callback`.

- Output model: `Acurite-5n1`; integer 12-bit ID; channel string A/B/C.
- Message `0x31` (49): `wind_avg_km_h`, `wind_dir_deg`, cumulative `rain_in`.
- Message `0x38` (56): `wind_avg_km_h`, `temperature_F`, `humidity`.
- `battery_ok`, `message_type`, `sequence_num` are metadata; repeated copies
  are not independent measurement cycles.
- Rain extraction uses 14 bits at 0.01 inch/count. Its representable range is
  not Atlas's 9-bit counter; physical reset/wrap behavior still needs testing.
- Caller rejects bad additive checksum, parity and invalid channel. Direction
  uses a lookup table; source notes historical mapping differences.
- Normalize wind km/h to mph; merge the two packet types by bound identity.
  Missing UV/light is an expected capability, not a failed complete cycle.

### AcuRite Notos / 3-in-1

Source: `acurite.c`, `acurite_3n1_decode` and its calling branch.

- Output model: `Acurite-3n1`; 14-bit ID, channel A/B/C; type `0x20` (32).
- One record contains `temperature_F`, `humidity`, `wind_avg_mi_h`, battery.
  No rain, wind direction, UV or light.
- Caller checks length and additive checksum; it deliberately does not apply
  the general parity gate because of conflicting fixtures.
- Source flags `sequence_num` as questionable: its bits overlap the ID.
  Never use that field as independent proof that a new station cycle occurred.
- Enable a three-measurement profile; do not require Atlas message types.

### Sainlogic / Gevanti SA8

Source: `sainlogic_sa8.c`, `sainlogic_sa8_decode`.

- 433.92 MHz OOK/PCM, UART-coded bytes. Decoder looks for a preamble and
  extracts 41 bytes; this is not AcuRite PWM.
- Output model `Sainlogic-SA8`; ID is a 12-character hexadecimal **string**
  representing six bytes. Do not truncate into Atlas's numeric ID field.
- Actual `data_make` emits `temperature_C`, `humidity`, `wind_avg_km_h`,
  `wind_max_km_h`, `wind_dir_deg`, `rain_mm`, `battery_ok` and `counter`.
  Despite the wind variable names and declared `.fields` list, the emitted
  wind keys/values are km/h. Integrate the actual output, not the field list.
- Rain: 16-bit raw value times **0.42893617 mm** in source. This needs measured
  verification against the console/manual before trusted accumulation.
- CRC-16 polynomial 0x8005, initial 0xffff, checks 36 bytes beginning at byte 3.
  **Mismatch logs an error but does not return failure.** The emitted `mic: CRC`
  must not be treated as verified integrity until fixed and negatively tested.
- Counter can inform cycle tracking after verified semantics; station-reported
  gust remains distinct from our maximum received wind sample over five minutes.

### Fine Offset WH24 / WH65 / WS69

Source: `fineoffset.c`, `fineoffset_WH24_callback`.

- FSK; actual output models `Fineoffset-WH24`, `Fineoffset-WH65B`,
  `Fineoffset-WS69`. The string WH65B is used by the decoder across RF variants;
  it does not prove a 915 MHz receiver profile.
- Eight-bit ID changes on battery replacement per source. Discovery must allow
  deliberate re-binding rather than silently changing an owner's station.
- Outputs Celsius, percent humidity, `wind_avg_m_s`, `wind_max_m_s`, degrees,
  `rain_mm`, `light_lux`, raw `uv` and derived **`uvi`**. Prefer `uvi` for a UV
  index; Atlas's `uv` naming cannot be generalized to this protocol.
- Wind factors differ by inferred WH24/WH65 type. Rain uses a 16-bit counter:
  WH24 0.3 mm/count; WH65/WS69 branch 0.254 mm/count.
- Base data requires CRC-8 and additive checksum. WS69's optional pressure
  extension has a separate check; a bad extension suppresses pressure while
  retaining base weather. Respect conditional output and invalid sentinels.
- Device classification includes packet-length heuristics. Check the actual
  delivered revision rather than assuming the inferred model is infallible.

### Fine Offset WH1080 / WH3080

Source: `fineoffset_wh1080.c`.

- OOK and FSK decoder variants; RF-band variants also exist.
- Model `Fineoffset-WHx080`; `subtype` selects weather, UV/light or radio clock.
  Do not treat a clock-only record as a fresh temperature observation.
- Weather gives `temperature_C`, `humidity`, wind average/max km/h, degrees,
  cumulative `rain_mm` and battery. Rain extraction is 12 bits, 0.3 mm/count.
- CRC failures return `DECODE_FAIL_MIC`. Merge weather and UV/light updates
  without refreshing fields missing from that subtype.

### La Crosse WS-2310 / WS-3600

Source: `lacrossews.c`.

- Explicit 433 MHz OOK. Models `LaCrosse-WS2310` / `LaCrosse-WS3600`, numeric ID.
- Temperature, humidity, rain, wind and gust are separate decoded records.
  Wind is m/s, direction degrees; cumulative rain is raw 12-bit value times
  0.5180 mm. Missing-wind error code is rejected, not reported as calm.
- The decoder validates checksum/parity but does not emit a `mic` key in these
  output paths. A universal `mic`-required rule would reject legitimate data.

### Cotech / documented Sainlogic FT0300 family

Source: `cotech_36_7959.c`.

- Explicit 433 MHz OOK/Manchester; model `Cotech-367959`, ID changes on reset.
- Emits Fahrenheit (the local variable is misleadingly called `temp_c`),
  humidity, cumulative rain mm, wind average/max m/s, direction, battery,
  conditional `light_lux` and `uvi`.
- 112-bit frame; CRC-8 poly 0x31/init 0xc0 rejected on failure. Rain extraction
  uses 12 bits at 0.1 mm/count. Missing light/UV is omitted on some rebrands.

### Low-integrity sensors need a separate policy

`baldr_therm.c` requires repeated matching rows and constant-bit sanity checks,
but has no CRC. `nexus.c` similarly has no real checksum and contains explicit
collision rejection. `baldr_rain.c` is disabled for missing validity/checksum.
Repeated receptions reduce some false detections but do not replace integrity
checks. Keep such candidates marked low confidence; do not silently apply the
same export policy as integrity-validated weather stations.

## Confirmed-owner discovery during setup

Owner requirement: listen during setup, recognize several packets from the same
device, then ask whether that device belongs to the user. Manual selection
remains available under Advanced. The owner chose automatic detection as the
default: normal setup must never require knowing or typing a sensor ID.
The owner approved an automatic-detection prompt explaining
that detection can take up to five minutes. The following is the proposed
implementation contract:

Setup prompt: **Automatically find your weather station?** “Detection can take
up to 5 minutes. Keep your outdoor sensor powered on.” Primary action: **Find my
station**. Put manual configuration under **Advanced**. During the scan, show elapsed/progress, candidates
and a Cancel action. Five minutes is the scan timeout, not a guarantee that a
station will be found.

1. Start an explicit scan using validated 433 MHz profiles. Initially OOK;
   FSK requires a separately validated receiver profile. Show mode and elapsed
   scan time. Do not clear the existing binding until a replacement is confirmed.
2. Group candidates by decoder/model, full untruncated sensor ID, channel if
   present and receiver profile. Retain a bounded list (proposed maximum 16).
   Mark ambiguous identities rather than automatically merging separate devices.
3. Count at least **three independent, valid transmissions** as a suggestion
   threshold. Deduplicate repeated RF copies within a burst. Do not require a
   changed temperature/wind value: stable weather still produces valid updates.
   Packet repetition inside a single transmission does not count as three sightings.
4. Allow **up to five minutes total per scan**, offering a candidate earlier
   when it meets the threshold. Initially scan the validated 433 MHz OOK profile.
   SA8's 60-second cadence plus packet loss means a short scan may find too
   little. If no eligible device is confirmed by timeout, offer **Scan again** /
   **Set up manually**. Future multi-profile scans must fit the stated budget or
   explicitly offer a separate scan; avoid rapid OOK/FSK alternation that gives
   neither adequate listening time. A candidate must remain fresh when selected.
5. Show a candidate card led by human-readable family and live readings with
   units so the user can recognize their station. Keep ID/channel, last heard,
   independent sighting count and decoder/integrity confidence in expandable
   details. Show available measurements. Ask “Is this your weather station?” with
   Use this station / Keep looking. Permit comparing multiple nearby candidates.
   Capture and save identity automatically; no ID entry is needed in this flow.
6. A confidently decoded but unintegrated device may be shown as **Detected;
   integration unavailable**. Only a validated adapter can be selected for normal
   operation. A successful generic JSON decode is not permission to upload it.
7. Confirmation saves the exact identity and profile. Discard discovery samples
   from operational history/exporters; require fresh post-binding observations
   satisfying that profile's field and integrity rules before uploading.
8. Stations with separate wind/rain/thermometer transmitters need an explicit
   sensor-association step. Do not assemble a synthetic station from nearby
   devices solely because they share a brand or channel.

This is **discovery of protocol identity**, not reliable identification of the
printed retail console model: some rebrands share identical packets. If a
sensor omits a unique ID or changes ID on reset, tell the user and require an
explicit association/re-binding policy.

## Filter at both radio and application levels

The CC1101 is currently initialized at 433.92 MHz. That selects a receive band,
not a decoder allowlist. Build a reviewed list of 433 MHz protocol decoders,
then accept only the confirmed bound identity during normal operation. Keep
the receiver profile with each candidate; JSON model strings usually contain
no RF-frequency proof. Nearby 433-band center frequencies and mode parameters
may need different profiles even when all products are called “433 MHz.”

One demodulator cannot receive OOK and FSK simultaneously. The library supports
both but labels FSK experimental in its header. Discovery can eventually scan
profiles sequentially; normal operation should lock to the selected station's
validated profile. Benchmark decoder CPU, queue drops and memory before enabling
the entire OOK catalog. Broad source availability does not imply zero runtime cost.

## Validation still needed after source review

Source establishes packet layout, emitted fields and implemented checks. It
does not establish real RF reception with our board, manufacturing revisions,
actual sensor accuracy, undocumented resets, or station-reported gust intervals.
Use complete-cycle captures and malformed-frame fixtures to validate each
adapter; compare rain and units with the owner's console. Replay stays outside
the archive and all external uploads.

Source links: [pinned device implementations](https://github.com/NorthernMan54/rtl_433_ESP/tree/f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8/src/rtl_433/devices),
[receiver library](https://github.com/NorthernMan54/rtl_433_ESP/blob/f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8/README.md).
