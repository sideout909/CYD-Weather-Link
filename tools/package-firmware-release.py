#!/usr/bin/env python3
"""Create OTA release assets locally; never publish or include device settings."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True, help='Public GitHub owner/repository')
    parser.add_argument('--binary', type=Path, default=ROOT/'firmware/.pio/build/panellan_sc05_x/firmware.bin')
    parser.add_argument('--output', type=Path, default=ROOT/'dist')
    args = parser.parse_args()
    if len(args.repo)>180 or not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', args.repo) or any(p in ('.', '..') for p in args.repo.split('/')):
        parser.error('--repo must be owner/repository')
    version = re.search(r'current="([0-9]+\.[0-9]+\.[0-9]+)"', (ROOT/'firmware/include/firmware_version.h').read_text())[1]
    if any(len(p)>5 or int(p)>65535 for p in version.split('.')):
        parser.error('Version components must fit the updater’s 0–65535 range')
    data = args.binary.read_bytes()
    if not 32768 <= len(data) <= 0x330000 or data[0] != 0xe9 or int.from_bytes(data[12:14], 'little') != 9 or int.from_bytes(data[32:36], 'little') != 0xabcd5432:
        parser.error('Binary is not a slot-sized ESP32-S3 application image')
    # Refuse packaging a stale build after editing the version header.
    if version.encode()+b'\0' not in data:
        parser.error('Rebuild firmware: current version was not found in the image')
    filename = 'weather-link-panellan_sc05_x.bin'
    manifest = dict(schema=1, board='panellan_sc05_x', partition_layout='default_8MB-v1', flash_bytes=8388608,
                    version=version, size=len(data), sha256=hashlib.sha256(data).hexdigest(),
                    firmware_url=f'https://github.com/{args.repo}/releases/download/v{version}/{filename}')
    args.output.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.binary, args.output/filename)
    (args.output/'weather-link-panellan_sc05_x.json').write_text(json.dumps(manifest, indent=2)+'\n')
    shutil.copyfile(ROOT/'assets/fonts/LICENSE-DejaVu.txt', args.output/'LICENSE-DejaVu.txt')
    shutil.copyfile(ROOT/'assets/web/leaflet/LICENSE', args.output/'LICENSE-Leaflet.txt')
    shutil.copytree(ROOT/'third_party/licenses', args.output/'third-party-licenses', dirs_exist_ok=True)
    shutil.copyfile(ROOT/'third_party/README.md', args.output/'THIRD-PARTY-NOTICES.md')
    if (ROOT/'LICENSE').exists():
        shutil.copyfile(ROOT/'LICENSE', args.output/'LICENSE')
    print(f'Prepared v{version} release assets in {args.output}; upload them to a GitHub Release tagged v{version}. Nothing published.')


if __name__ == '__main__':
    main()
