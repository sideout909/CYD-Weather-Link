# Device recording and seven-day export

Version 0.4.0 keeps the existing rolling 24-hour, one-minute display history
and adds a separate seven-day archive of five-minute summaries. Recording
uses internal flash; it needs no PC or SD card. Keep the device powered and
allow its configured Wi-Fi/NTP connection to establish UTC after a cold boot.
The archive starts with this firmware; older days cannot be reconstructed.

## Retention and measurements

The fixed archive has 2,016 slots of 64 bytes: 129,024 bytes (126 KiB), plus
filesystem overhead. UTC five-minute intervals select slots; the oldest slot
is replaced as the ring advances. Offline/reception gaps are never backfilled
with invented weather. An empty interval during powered operation retains
reception/coverage counts, while time when the device was off appears as a gap.

Each record includes:

- UTC interval start and bound Atlas sensor ID/channel.
- Temperature mean/min/max, mean humidity, mean/peak lux, and mean/peak received
  wind speed. Means use accepted samples; the wind peak is the maximum sample
  in that fixed interval, separate from the Home rolling five-minute gust.
- Observed rain-counter increments, sample counts and a rain-completeness flag.
  Counter gaps/resets or rainfall straddling a boundary remain uncertain;
  observed rain is not presented as a complete total when that flag is false.
- Accepted/repeated/rejected bound-station frames, observed seconds, seconds
  classified Current, partial-interval and saturated-count flags.

Missing measurements export as blank cells; genuine zero remains zero. Frame
counts refer to decoded packets delivered to the application, not every RF
transmission in the air. Current seconds use the existing station-health test,
not an independent calibration/accuracy assessment. CSV times are UTC.

## Persistence and headroom

The archive writes a completed summary at each UTC five-minute boundary.
A controlled settings/OTA restart saves the partial interval. Sudden power loss
can lose the current interval (up to five minutes). Reopening a saved partial
merges earlier counts/means conservatively and keeps partial/rain-gap flags.
Small rounding differences in resumed means are possible from their stored
precision. The CRC rejects torn/corrupted records independently; other valid
slots remain readable. Different station IDs/channels are not mixed.

The archive file is preallocated once. Each commit updates one logical 64-byte
slot; LittleFS adds flash-block/metadata write overhead. It is not a repeated
whole-week snapshot. Allocation and archive writes preserve a 512 KiB free-space
reserve. The 1.5 MiB data partition, existing weather checkpoint and NWS caches
are unchanged, as are both application/OTA slots. Storage errors/free space
appear in the portal and sanitized USB status. The seven-day archive and the
existing weather checkpoint are independent; changing the archive did not
change checkpoint schema 3 or its 24-hour restore limit.

## Download

In the authenticated setup page's Overview, use **Download seven-day CSV**.
The browser requests small pages, allowing the main loop to process RF and
touch between requests. Existing 24-hour charts retain their scale. No archive
upload to a cloud service is added, and no account keys, callsign or coordinates
are stored in archive records. The archive endpoint uses the same admin login
as setup. Existing HTTP transport limitations still apply.

With USB connected to the PC, from the repository root:

```bash
bash tools/firmware-console.sh archive captures/device-seven-day.csv
bash tools/firmware-console.sh status
```

USB download fixes a pagination window and writes a `.partial` file until the
whole CSV succeeds. To checkpoint live data and the current partial interval
before a controlled bench restart:

```bash
bash tools/firmware-console.sh checkpoint-save
```

Replay/test samples never enter production archives. Archive downloads are
read-only. A download across ring rollover can omit an oldest slot overwritten
while that download is in progress; timestamps expose that gap. Preserve a
successful CSV on the PC if you want a permanent record beyond the rolling week.

## Validation

Host checks cover schema size, means/peaks, zero/missing, coverage/gaps, expiry,
CRC, station binding, rain boundaries, partial restart merging and saturation.
Python checks cover pagination/window validation and complete-file publication;
browser checks exercise CSV assembly and a failed page/retry. Actual board
results and remaining soak/power-interruption limits are in VALIDATION.md.
