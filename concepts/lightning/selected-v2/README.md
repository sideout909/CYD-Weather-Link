# Selected option B — alternating lightning / Barometer tile

Owner selected option B on October 22. This supersedes the original recommended
strip. [Interactive concept](index.html) · [Both phases](proposal.png).
No firmware changes or device access; installed firmware remains untouched.

## Selected behavior

- Use the existing Barometer tile location. Preserve light/UV, other Home
  readings and the full NWS alert area.
- When validated lightning data changes, show lightning first for five seconds,
  then Barometer for five seconds, repeating while eligible.
- Lightning tile includes the bolt, count over the proposed ten-minute window,
  **estimated distance of the last strike**, and last-strike age. It is not the
  closest-ever strike distance or the distance to a forecast storm.
- At **30 minutes with no real lightning-data change**, consider the tile stale
  for Home, stop alternating and show only Barometer. Identical received packets
  do not reset this timer. This is separate from receiver/field freshness states.
- Track real counter activity and valid last-distance changes. Keep the
  last-data-change time separate from last-strike time: a distance correction
  can restart display eligibility without adding a strike or making its age zero.
- No lightning sensor/capability: Barometer only; no extra placeholder/toggle.
  First counter sample is a baseline, not proof of a recent event. Resetting or
  rebinding the sensor must not fabricate a strike or activate old activity.
- Missing/unknown distance shows a dash, not zero miles. It does not count as
  new valid lightning activity. Communication loss must not be presented as
  current live lightning; detailed status retains field age and source.
- Display expiry hides the tile; it does not erase event history or mean the
  area is safe. Future detailed history can retain age/coverage information.

## Distance research

The pinned `acurite.c` Atlas packet legend identifies the distance field in
**miles** (`D` in the final lightning bytes). The decoder exposes five raw bits
as `strike_distance`. Imperial display uses miles; metric display converts with
1 mile = 1.609344 km. Do not make units follow arbitrary decoder defaults.

[Upstream Atlas protocol documentation](https://github.com/merbanan/rtl_433/blob/master/src/devices/acurite.c)
contains that miles legend, but also retains a TODO beside the raw extraction.
That means special/reserved codes and real display agreement still need fixtures
or live comparison; the legend does not establish every raw value's meaning.
The manufacturer describes an estimated latest-strike distance, with a nominal
25-mile (40-km) detection range, in its
[Atlas station description](https://support.acurite.com/hc/en-us/articles/360021588613-AcuRite-Atlas-7-in-1-Weather-Station).
Do not interpret all 0–31 codes as validated distances just because they fit.

The mock uses a fictional 8 mi / 12.9 km estimate. Actual firmware distance
handling is still pending, along with counter wrap/reset and observation-window
coverage validation. Unknown sentinels stay unknown until resolved.

## Prototype controls

The clock alternates every five real seconds. Buttons simulate a new strike,
identical retransmission, valid distance correction, 29/30 minutes without
change, an unknown distance, metric presentation and absent capability. Only
prototype values change; no network, USB or device operation exists in this page.
The last-strike age and ten-minute event window use a separate simulated event
clock so a distance-only correction cannot fabricate a new strike.

Regenerate from repository root:
`python3 concepts/lightning/selected-v2/generate.py`.
