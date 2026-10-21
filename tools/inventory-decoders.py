#!/usr/bin/env python3
"""Inventory r_device declarations in the locally pinned rtl_433_ESP source.

This is a source inventory, not an RF compatibility test or a C preprocessor.
Frequency mentions are file-level evidence only, never an automatic band filter.
"""
import argparse
import csv
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REVISION = "f0a81569b58a60ea8b3a5efd8e9b39670e8d8df8"
BASE = f"https://github.com/NorthernMan54/rtl_433_ESP/blob/{REVISION}/src/rtl_433/devices/"
WEATHER = re.compile(
    r"weather|temperature|thermo|humidity|hygro|rain|anemometer|wind|lightning|"
    r"acurite|oregon|bresser|fine.?offset|lacrosse|la crosse|sainlogic|ecowitt|"
    r"soil|moisture|solar|uv sensor", re.I)

# Reviewed model scopes, not an inferred band for everything in a source file.
# Keep retail RF evidence and unresolved revisions in DECODER_SUPPORT.md.
BAND_433 = {
    "acurite_txr": "Atlas, Iris/5-in-1 and Notos/3-in-1 433 MHz families; other TXR models require individual band review",
    "sainlogic_sa8": "Sainlogic SA8 / documented Gevanti SA8 protocol; CRC fix required",
    "fineoffset_WH25": "WH24/WH65/WS69/MISOL WS2320 verified 433 MHz variants only",
    "fineoffset_wh1080": "WH1080/WH3080 verified 433 MHz OOK variants only",
    "fineoffset_wh1080_fsk": "WH1080/WH3080 verified 433 MHz FSK variants only",
    "ambient_weather": "F007TH/TFA/SwitchDocLabs protocol, verified 433 MHz variants only (also 868)",
    "archos_tbh": "TBH sensor documented at 433.93 MHz",
    "auriol_afw2a1": "Auriol AFW2A1",
    "cotech_36_7900": "Cotech 36-7900 rain gauge",
    "cotech_36_7959": "Cotech 36-7959 / documented FT020T, WS019T, FT0300, FT-0310, 0366 rebrands; confirm revision",
    "emos_e6016": "EMOS E6016 / E6018",
    "emos_e6016_rain": "EMOS E6016 rain gauge",
    "esperanza_ews": "Esperanza EWS-103",
    "gt_wt_02": "Globaltronics GT-WT-02",
    "gt_wt_03": "Globaltronics GT-WT-03",
    "homelead_hg9901": "Homelead HG9901 documented EU 433.92 MHz variant",
    "inkbird_ith20r": "Inkbird ITH-20R",
    "kedsum": "Kedsum / Pearl NC-7415; documented units around 433.71 MHz",
    "lacrossetx": "La Crosse TX 433 MHz OOK family only",
    "lacrossews": "La Crosse WS-2310 / WS-3600",
    "rainpoint": "RainPoint soil temperature/moisture protocol documented at 433.9 MHz",
    "rainpoint_hcs012arf": "RainPoint HCS012ARF rain gauge",
    "schou_72543_rain": "Schou 72543 and documented Motonet/MarQuant/TFA/ADE rain-thermometer rebrands",
    "telldus_ft0385r": "Telldus FT0385R sensors",
    "ttx201": "EMOS TTX201",
    "vauno_en8822c": "Vauno EN8822C",
    "wssensor": "Hyundai WS SENZOR",
}


def uncomment(text):
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)


def inventory(source):
    records = []
    for path in sorted(source.glob("*.c")):
        original = path.read_text()
        clean = uncomment(original)
        arrays = {
            m[1]: re.findall(r'"([^"\n]*)"', m[2])
            for m in re.finditer(
                r"(?:static\s+)?(?:char\s+const|const\s+char)\s*\*\s*const\s+"
                r"(\w+)\s*\[\s*\]\s*=\s*\{(.*?)\};", clean, re.S)
        }
        # Mentions can belong to another decoder/model in this file.
        mentions = [f"{number}: {line.strip()}" for number, line in
                    enumerate(original.splitlines(), 1)
                    if re.search(r"\b\d{3}(?:\.\d+)?\s*MHz\b|\b-f\s+\d{3}(?:\.\d+)?M\b",
                                 line, re.I)]
        for match in re.finditer(
                r"r_device\s+const\s+(\w+)\s*=\s*\{(.*?)\};", clean, re.S):
            body = match[2]
            def value(key):
                found = re.search(r"\." + key + r"\s*=\s*([^,\n]+)", body)
                return found[1].strip() if found else ""
            name_match = re.search(r'\.name\s*=\s*"([^"\n]+)"', body)
            if not name_match:
                raise ValueError(f"Missing name: {path.name}:{match[1]}")
            name = name_match[1]
            fields = arrays.get(value("fields"), [])
            records.append({
                "symbol": match[1], "name": name, "source": path.name,
                "source_url": BASE + path.name,
                "modulation": value("modulation"),
                "disabled_initializer": value("disabled") or "0 (implicit)",
                "decode_function": value("decode_fn"),
                "declared_fields": fields,
                "weather_candidate": bool(WEATHER.search(name) or any(
                    re.match(r"temperature_|humidity$|rain_|wind_|strike_|moisture", f)
                    for f in fields)),
                "frequency_evidence_scope": "file only; not a model band assignment",
                "file_frequency_mentions": mentions,
            })
    expected = {m[1] for path in source.glob("*.c") for m in re.finditer(
        r"r_device\s+const\s+(\w+)\s*=", uncomment(path.read_text()))}
    if {r["symbol"] for r in records} != expected:
        raise ValueError("Inventory did not capture every r_device declaration")
    if not records or len({r["symbol"] for r in records}) != len(records):
        raise ValueError("Empty inventory or duplicate decoder symbols")
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT /
                        "firmware/.pio/libdeps/panellan_sc05_x/rtl_433_ESP/src/rtl_433/devices")
    parser.add_argument("--output", type=Path, default=ROOT / "reference/decoder-inventory")
    args = parser.parse_args()
    records = inventory(args.source)
    symbols = {r["symbol"] for r in records}
    if not BAND_433.keys() <= symbols:
        raise ValueError("A reviewed 433 MHz entry is missing from this library")
    args.output.mkdir(parents=True, exist_ok=True)
    payload = {"library_revision": REVISION,
               "limitations": "Source declarations, not enabled firmware support. Declared fields are optional unions. File frequency mentions are not per-model RF specifications. Weather selection is heuristic. Preprocessor conditions are not evaluated.",
               "decoders": records}
    (args.output / "all-decoders.json").write_text(json.dumps(payload, indent=2) + "\n")
    columns = ["symbol", "name", "modulation", "disabled_initializer", "declared_fields",
               "source_url", "file_frequency_mentions"]
    for filename, rows in [("all-decoders.csv", records),
                           ("weather-candidates.csv", [r for r in records if r["weather_candidate"]])]:
        with (args.output / filename).open("w", newline="") as handle:
            writer = csv.DictWriter(handle, columns, extrasaction="ignore")
            writer.writeheader()
            for row in rows:
                writer.writerow({**row, "declared_fields": "; ".join(row["declared_fields"]),
                                 "file_frequency_mentions": " | ".join(row["file_frequency_mentions"])})
    with (args.output / "433-weather.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, ["model_scope", *columns], extrasaction="ignore")
        writer.writeheader()
        for row in records:
            if row["symbol"] in BAND_433:
                writer.writerow({**row, "model_scope": BAND_433[row["symbol"]],
                                 "declared_fields": "; ".join(row["declared_fields"]),
                                 "file_frequency_mentions": " | ".join(row["file_frequency_mentions"])})
    print(f"Inventoried {len(records)} decoder declarations; "
          f"{sum(r['weather_candidate'] for r in records)} weather/environment candidates; "
          f"{len(BAND_433)} reviewed 433 MHz decoder entries (some variant-restricted).")


if __name__ == "__main__":
    main()
