# Experimental daylight cloud estimate

Implemented in 0.8.0-rc.1; disabled until enabled and calibrated. This is a local
sky estimate, not cloud cover percentage, a forecast, or a measurement for uploads.
It changes only the current Home/Overview sky icon. Forecast icons remain NWS.

## Research and limits

[NOAA solar-position equations](https://gml.noaa.gov/grad/solcalc/solareqns.PDF)
provide fractional-year declination, equation of time and elevation from UTC,
latitude and longitude. Thus the expected daytime light changes with hour,
latitude and season rather than using a fixed lux threshold. At solar elevation
below 15° the classifier stops; this excludes night and much of the dawn/dusk
transition, including winter low-sun cases. Sunrise/sunset still choose sun/moon
for NWS fallback graphics.

A Haurwitz-like relative curve, `sin(elevation) * exp(-0.059/sin(elevation))`,
normalizes a site calibration to 45° solar elevation. See the clear-sky discussion
in [surface-flux research](https://edepot.wur.nl/203009). Its use here for relative
illuminance is a project heuristic, not a validated lux or cloud model. We do not
convert lux to solar watts: [BIPM photometry principles](https://www.bipm.org/documents/20126/41749107/Principles%2B-governing%2B-photometry/8365350c-bf72-3cf2-bf03-324bf5c01b62)
describe lux as spectrally weighted illuminance.

The owner observed roughly 15 klux in direct midday fall sun earlier in the
project. That is a local observation, not a universal clear-sky threshold. Mount
height is 30 ft with full-sun exposure; sensor orientation, shading, haze,
reflecting snow, dirt and atmospheric conditions still matter. Cloud edges may
increase light. No atmospheric/turbidity model or seasonal calibration dataset
has been validated. Recalibrate after moving/cleaning/changing the sensor; location
or station changes invalidate calibration unless explicitly supplied anew.

## Settings and behavior

In Station & Wi-Fi, enable the experimental daylight estimate and check the
clear-direct-sun confirmation only when it is true, then Save and restart. The
server requires unchanged station/location, fresh live light, a valid clock,
non-saturated readings and solar elevation at least 15°. Calibration persists in
project settings. The hidden API reference value is lux normalized to 45°; normal
users need not enter a number.

Each distinct RF light timestamp contributes to a one-minute exponential smoother.
Ratios below 0.4 suggest cloudy, below 0.8 partly cloudy, otherwise clear. These
broad thresholds are experimental project choices. A category needs at least four
distinct samples spanning two minutes before becoming stable; repeated polls
cannot build evidence. A previous stable category remains during a transition.
Replay, missing/future/older-than-two-minute light, low sun and invalid calibration
immediately remove the local estimate. Values at/above 200 klux or more than 1.6
of the clear reference also fall back rather than claiming confidence.

A valid estimate replaces the source label with **LUX** centered above the icon on the display and **LUX*** on the webpage.
Otherwise the existing fresh NWS icon/source rules apply. At night the actual moon
phase and NWS overlays remain. Alerts and all telemetry/export validity are
independent of this classifier. Setup health exposes reason, elevation, expected
light and ratio to help investigate field performance.

## Initial owner calibration

On October 22 the owner confirmed no clouds all day and authorized use of today's
recordings. A median of 153 daylight minute samples, normalized by solar elevation
and excluding sun below 15°, produced a 45° reference of 50,293.8 lux. It was saved
on the owner's device and the option enabled. The older 15 klux anecdote was not
used as a universal calibration. Private raw recordings/location stay ignored.

Host tests cover seasons, equatorial noon/night, invalid coordinates, calibration,
transitions, duplicate timestamps, sparse/stale/replay/future inputs and saturation.
Warmup uses the same anchored millisecond snapshot clock as RF fields; whole-second
wall-clock rounding was found to reset legitimate subsecond samples during the
first bench attempt and was corrected before final acceptance. Sample count and
evidence span are visible in setup health. Browser tests cover LUX* attribution and return to NWS. Multi-day field comparison,
cloudy/clear transition accuracy and seasonal performance remain owner acceptance
work; these are not yet validated by the current clear-day calibration.


Final live check: after clock-basis correction, five distinct samples spanning
122 seconds produced Likely clear, and six samples spanning 184 seconds retained
it. RF packets increased, fresh AWC pressure and APRS remained healthy. Owner's
final display refinement centers LUX above the sun without an asterisk; webpage
keeps LUX*. Forecast/current-source distinction is otherwise unchanged.
