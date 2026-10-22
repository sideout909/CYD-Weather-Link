# Optional lightning on Home — original exploration

**Owner selected option B with revisions.** See the [selected v2 concept](selected-v2/index.html)
and [behavior/distance research](selected-v2/README.md): alternate with Barometer
every five seconds, include last-strike distance, expire after thirty minutes
without a real lightning-data change. The A/B notes below are the earlier study.

[Interactive preview](index.html) · [Comparison PNG](comparison.png) ·
[Editable comparison SVG](comparison.svg). Open locally; no network or device needed.

These reuse the original Home v2/theme SVG geometry at native 320×240. The
readings, callsign, lightning activity and NWS warning are fictional. Source
is editable vector code, not an altered device framebuffer. No firmware change,
flash, publication or claim of live lightning reception is made.

## A — contextual strip, recommended

Keep light/UV under the temperature graph normally. During a defined recent
activity period, use that row for an amber bolt and `4 strikes / 10m · 42s ago`.
The temperature, wind, three measurement tiles and full 44-pixel NWS alert target
keep their space. Return to light/UV once the recent-event hold expires. Suggested
initial concept: ten-minute count, with last-event age retained for a thirty-minute
hold; these are design proposals, not current firmware behavior. Light/UV stays
available on Outdoor/status while the strip is showing. A future large detail
target can expose the event history; this 15-pixel strip is not a tiny button.

## B — optional tile

An explicitly chosen lightning tile replaces Barometer for a station that reports
lightning capability. Show ten-minute count and last-event age, or a dash/data-old
message. Keep pressure on Outdoor/status. If no lightning capability is reported,
keep Barometer and the normal layout. This trades always-visible pressure for
always-visible lightning; it should not be the default for everyone.

## Capability and data rules

- Most stations will not have lightning. Infer support only from validated native
  fields on the confirmed bound station, not from the brand/model name or another
  nearby station. Atlas itself may lack the optional lightning hardware; see
  [AcuRite sensor installation](https://support.acurite.com/hc/en-us/articles/1260802078810-AcuRite-Atlas-7-in-1-Sensor-Installation).
- Auto hides all lightning UI until capability is observed; no blank placeholder,
  disabled badge or fake zero on normal Home. If a model explicitly lacks it,
  leave Home unchanged. Unknown/not-yet-reported is not proof of absence.
- Once capability has been observed, loss of that field is stale/unknown, even if
  temperature and RF packet ages remain current. Do not silently turn it into zero.
  Reset/rebind capability appropriately when the selected station changes.
- A first counter sample establishes a baseline; it is not an event. Repeated
  counter values do not add strikes. A valid zero ten-minute total requires a
  sufficient fresh observation window; gaps/resets/uncertain wraps stay incomplete.
- The pinned Atlas decoder extracts a nine-bit counter, but reset/wrap behavior
  and plausible increments need review/fixtures before any rolling count exists.
  Avoid interpreting a power reset as hundreds of strikes. Persist event history
  and baselines deliberately; missing history must not imply complete coverage.
- Last-event age is based on when a new counter increase is received locally;
  without a sensor timestamp it is not exact strike time. State this in details.
- Do not map a raw distance code to miles/km yet: the pinned decoder has an
  explicit distance-conversion TODO. Distance, signal strength and bearing must
  not be invented. No distance appears in these concepts.
- Keep local detection separate from NWS warning/watch state. The NWS card keeps
  its semantic colors, headline and touch area. A lightning counter does not
  create an NWS warning or replace a full-screen warning presentation.
- Normal/quiet is not an all-clear. Do not treat missing data as no lightning.
  Derive per-field freshness thresholds from real transmission cadence before
  implementation. Preserve night-theme brightness, avoid flashing or sound.

## Existing source and missing implementation

`firmware/include/atlas_packet.h` already loads native `strike_count` into the
weather sample and `weather_store.h` retains the field. The native decoder also
emits `strike_distance` with unresolved units. There is no validated rolling
lightning event window or Home lightning rendering added by this concept.

Implementation would need capability tracking, counter delta/reset/wrap handling,
coverage/freshness and source binding, bounded event storage, detail presentation,
unit/cadence review and fixtures. Future non-Atlas adapters can publish this
capability when supported; they should not have to reserve a main-screen tile.

Regenerate: `python3 concepts/lightning/generate.py` (standard library plus
`rsvg-convert`). SVGs come from the existing Ice Blue palette study; icon shapes,
weather layout and fictional readings are kept consistent across the options.
