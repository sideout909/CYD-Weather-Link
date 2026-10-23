# Signed OTA and update channels — local 0.9.0-rc.1

Owner authorized prerelease publication and device installation. Stable Latest
remains 0.8.2; 0.9.0-rc.1 is for further field testing, not production validation.
The private bootstrap/hosted signed OTA results are recorded in VALIDATION.md.
The device is being prepared for an overnight dim/touch recurrence check.

## Owner controls

Updates offers **Stable** (default, including migration) and **Dev** (explicit
opt-in, includes supported prereleases and stable releases). Save and restart to
apply selection. Stable uses GitHub Latest; Dev checks the newest ten public
releases and selects the greatest supported version, skipping drafts, malformed
tags and inconsistent prerelease flags. Rate limits and lookup failures remain
visible; no token is stored. Returning to Stable never installs an older image:
it waits for a newer stable version. Daily checks and automatic installation
remain separately opt-in; automatic installation retains NWS alert guards.

The repository field is read-only until **Enable changing repository** is
checked. The checkbox is not persisted and resets on page reload. Custom saved
repositories remain intact. Changing the repository cannot change publisher trust.
A fork must build with its own public trust key to deliver its own signed images.
USB/custom firmware remain available; no Secure Boot, flash encryption or eFuse
changes are involved.

## Signature contract

Firmware accepts only manifests signed by the public key compiled into
`include/ota_trust.h` (or the build-time `WX_LINK_OTA_TRUST_HEADER` override).
The ECDSA P-256 signature uses SHA-256; the existing SDK mbedTLS implementation
performs verification. Signature is DER encoded, represented as lowercase hex.
The key identifier is the first 16 bytes of SHA-256 of the public SPKI DER,
encoded as 32 lowercase hex characters. A release cannot supply a trusted key.

JSON retains base `schema: 1` so older updaters can read the bootstrap release.
New updaters additionally require `signature_schema: 1`,
`signature_algorithm: "ecdsa-p256-sha256"`, `signing_key_id`, `signature` and
`channel` (stable for final versions, dev for `-rc.N`). No unsigned fallback.
The canonical ASCII signature payload is these LF-separated lines, including
one final LF; unsigned integers use ordinary decimal without leading zeros:

```text
WX-Link OTA signature v1
ecdsa-p256-sha256
<signing_key_id>
<board>
<partition_layout>
<flash_bytes>
<version>
<channel>
<size>
<sha256>
<firmware_url>
```

Identifiers and lengths are bounded. The digest must be 64 lowercase hex
characters; version and release URL must match the selected repository. All
installation-relevant metadata is signed, including board/layout and image hash.
Signature verification precedes update availability. Existing board/layout/size,
HTTPS host restrictions, no-downgrade policy, streamed image SHA-256, Update image
validation and startup confirmation/rollback still apply. A changed image cannot
match the signed digest. Signature protection applies to software OTA, not physical
USB replacement or an attacker who already controls the running firmware.

## Publisher key and packaging

A publisher key was created outside the repository, with mode 0600:
`~/.local/share/wx-link/ota-signing/publisher-v1.key`. Only its public key is
tracked. Keep this key out of devices, source, logs, captures, ZIPs and GitHub.
Make an offline protected backup before distributing a release that trusts it.
It is an unencrypted local signing key; filesystem access protects the working
copy. Do not recreate/overwrite it after devices begin trusting it.

Generate a separate fork key only with an unused private path and output header:

```bash
python3 tools/create-ota-signing-key.py \
  --private-key /secure/path/publisher.key --header /path/to/fork/ota_trust.h
```

The tool refuses private keys under this checkout and refuses to overwrite keys
or headers. Encrypted/offline backups and release signing are publisher concerns;
no private signing key is needed to build or use the firmware.

```bash
bash tools/pio.sh run -d firmware
python3 tools/package-firmware-release.py --repo sideout909/WX-Link \
  --signing-key "$HOME/.local/share/wx-link/ota-signing/publisher-v1.key" \
  --output dist/v0.9.0-rc.1
```

Packaging requires a matching P-256 private key, compares it with the actual
public header, and checks version/key identity in the built image. It signs
before creating release output. Local artifacts remain ignored; no publication
is performed. The initial upgrade from 0.8.2 cannot enforce a signature because
that installed verifier predates this feature. Once this candidate is installed,
subsequent OTA checks require signatures. Bootstrap testing remains necessary.

Current trust contains one publisher key. Future rotation must first deliver an
old-key-signed transition build that trusts both old and new keys, then switch
signing, then retire old trust in a later signed build. Dual-key rotation is not
yet implemented. Lost/compromised-key recovery may require owner USB reflashing;
a configurable repository alone cannot recover trust. Keep key IDs distinct.

## Validation and remaining acceptance

SC05_X build, model checks and synthetic browser flow pass. The actual firmware
verifier was compiled against host mbedTLS 2.28.7 (the SDK version) and tested with
ephemeral keys: valid Stable/Dev and reordered JSON pass; changed metadata,
wrong repository/key, missing/invalid/truncated signatures and schema/algorithm
changes fail. Publisher private material is not used by this regression suite.

To reproduce, obtain official Mbed TLS v2.28.7 sources outside this repository:

```bash
cmake -S /path/to/mbedtls-2.28.7 -B /tmp/wxlink-mbedtls-build \
  -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build /tmp/wxlink-mbedtls-build --target mbedcrypto -j4
python3 tools/test-signed-ota.py --mbedtls-source /path/to/mbedtls-2.28.7 \
  --mbedtls-build /tmp/wxlink-mbedtls-build
```

Actual Stable/Dev lookups, unsigned Stable rejection, signed Dev verification,
bootstrap plus signature-enforced OTA, preference restoration and controlled
restart passed on the owner’s device. Post-restart free heap was 128,276 bytes
and loop-stack free space 9,900 bytes. Dedicated signature timing, extended
resource/soak, interrupted-transfer and deliberate failed-boot gates remain.
Do not label this prerelease production-validated or publish it as Latest.
A protected second local signing-key copy is retained outside the repository;
copy it to independent/offline storage as publisher recovery follow-up.

References: [GitHub release API](https://docs.github.com/en/rest/releases/releases),
[pinned mbedTLS public-key API](https://github.com/Mbed-TLS/mbedtls/blob/v2.28.7/include/mbedtls/pk.h).
