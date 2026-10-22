# Connect waveform review — October 22, 2026

Read-only review of the local WXLink Connect workspace. No Connect source,
settings, capture evidence or hardware was changed. Its agent has uncommitted
work; this review describes the files present now, not a published Connect release.

Ran `python3 tools/test-public-decoders.py` in the sibling Connect workspace:
**16 public pulse fixtures, 40 decoded callbacks passed** through its actual
pinned decoder copies and capture/preview integration. Registry contains 20 OOK
decoder symbols. All 20 decoder C sources match WX Link's pinned library byte
for byte; this does not include differing formatter/receiver/application code.

Public corpus: `merbanan/rtl_433_tests`, revision
`1aa8c29d6b9383739d1aeed495dbc5069b704392`. Fixture provenance, pulse hashes,
source paths and independent expected output are recorded in Connect's
`firmware/tests/fixtures/public-rtl433/provenance.json`; the audit is
`docs/DEEP_AUDIT_0.1.4.md`. Tests replay measured pulse/gap sequences extracted
from historical public IQ, not owner private recordings or synthetic RF.

| Priority | Models with passing public replay | Next production integration work |
| --- | --- | --- |
| First | AcuRite Iris/5n1 (3 packets), Notos/3n1 (2) | Decoder already enabled and discovery preview exists. Add explicit model-bound confirmation/persistence, station ID ranges, operational adapters and model-specific rain/cycle/freshness handling. |
| Next | AcuRite Tower (3), Ambientweather-F007TH (5), GT-WT02 (6), GT-WT03 (10) | Add bounded identity/discovery and temperature/humidity adapters. These are partial sensors; never invent wind/rain/light. Coordinate later multi-source composition. |
| Then | Fineoffset-WHx080 (1), Cotech-367959 (1), Auriol-AFW2A1 (1) | Review full packet fields, checksum, counter resolution/wrap, cadence, selected RF profile and all-in-one weather mapping. |
| Further evidence | Esperanza-EWS (1), Schou-72543 (1), Hyundai-WS (1), Vauno-EN8822C (1), Emos-TTX201 (1), LaCrosse-TX (2), LaCrosse-WS2310 (1) | Review each native schema, identity and measurement coverage before choosing adapter priorities. |

The replay count is small and only checks post-demodulator software. It cannot
prove CC1101 sensitivity, antennas, pulse capture timing, station variants,
transmission cadence or RF-loaded behavior. Some models only have one captured
packet. Live owner/community evidence remains necessary.

**No new operational station support is enabled by this review.** Atlas remains
operational; Iris/Notos remain detection previews. Reuse the evidence first for
production host fixtures, then implement explicit model-bound adapters with
export/history gates. Never feed discovery-only or unconfirmed samples into
history/exporters. Do not copy Connect's broad preview adapter into production:
its measurement preview is intentionally less strict than a weather uploader.
