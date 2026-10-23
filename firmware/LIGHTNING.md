# Optional lightning tile — 0.9.0-rc.1

The owner's selected option B is implemented in the local/prerelease source.
It uses the existing Barometer tile; light/UV and NWS warning presentation keep
their space. No extra hardware or required SD storage is introduced.

## Display rules

- Default is Barometer. A station without received lightning fields has no
  lightning placeholder. A first counter establishes a baseline, not an event.
- After a validated counter increase, alternate lightning and Barometer every
  five seconds. Continued activity does not restart that rotation and starve
  Barometer. A valid changed last-distance estimate can renew activity without
  adding a strike or changing last-strike age.
- After thirty minutes with no counter increase or valid distance change,
  stop alternating and show only Barometer. Repeated identical packets and
  unknown distance codes do not extend this activity timer.
- Lightning shows the ten-minute count, estimated last-strike distance and
  last-strike age. `*` in its heading means partial history. Counts above 99
  show `99+`; the full retained count is available in the detail page/USB status.
- Tap the lightning phase to open its detail page; tap the pressure phase for
  the existing pressure details. Unchanged packet-group diagnostics have been
  removed from Outdoor/status at owner request; APRS/report rows move up.
- Distance follows the existing metric preference: miles or converted kilometers.
  The protocol legend identifies miles. Only codes 1–25 are currently presented
  as distance; 0 and 26–31 remain unknown pending sentinel/edge validation.
  Unknown is `--`, not zero or a guessed out-of-range distance. See source below.
- Lightning-report freshness is independent from activity expiry: at two minutes
  without a valid lightning report, hide the tile rather than present it as live.
  Other weather packets cannot refresh this field. Detailed status retains age.
- Palette colors/backlight policy remain active, including Night. No sound,
  flashing, automatic wake, NWS warning creation or exporter changes are added.

## Input, counters and storage

`atlas_packet.h` accepts bounded integer `strike_count` (0–511) and
`strike_distance` (0–31). The receiver's existing native, confirmed-station,
channel, integrity/exception and store-acceptance gates run before the lightning
tracker. Unconfirmed, foreign, flagged and USB-replay samples never activate it.
Distance-only native changes are included in packet deduplication.

`lightning.h` keeps at most 64 timestamped, weighted counter-increase events.
The ten-minute window uses exact received timestamps; a new partial history is
not presented as complete. Gaps and counter decreases rebaseline conservatively.
Even 511→0 can be a reset: that ambiguous crossing is not invented as an event;
the next increasing counter resumes tracking. Overflow discards the oldest event
and marks coverage partial until discarded events leave the ten-minute window.

State is RAM-only, scoped to the selected station/session. Reboot/rebind/reset
starts a new baseline and partial window; it does not redisplay historical
strikes as current. Existing checkpoint/archive schemas and files are unchanged.
This is a bounded live activity view, not a persistent lightning history archive.
No received distance or strike value is sent to external weather services.

USB status adds a `lightning` object with capability, report freshness, activity,
rotation phase, count/partial coverage, optional distance in miles and event/change
ages. It contains no credentials or station identity. `tools/firmware-console.sh detail lightning` opens lightning (native detail 6);
the existing `detail 5` Outdoor alias remains compatible.

## Validation and limits

Host tests cover capability/initial baseline, repeats, non-starving five-second
rotation, exact thirty-minute expiry, distance correction versus event time,
unknown/range values, gaps, resets/ambiguous wrap, bounded overflow/coverage,
mi/km conversion and packed-font fit. Native adapter/store tests cover station/
channel isolation, exception rejection and malformed lightning fields. Firmware
build and inherited tests remain required. These do not prove live strike counts
or distance agreement during a storm. Real distance codes and counter behavior
need owner/community comparison with the Atlas display.

[Selected concept and distance research](../concepts/lightning/selected-v2/README.md),
[upstream protocol legend](https://github.com/merbanan/rtl_433/blob/master/src/devices/acurite.c),
[AcuRite Atlas description](https://support.acurite.com/hc/en-us/articles/360021588613-AcuRite-Atlas-7-in-1-Weather-Station).
