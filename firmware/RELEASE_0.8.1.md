WX Link v0.8.1 fixes Windy.com uploads that failed before reaching the service because of TLS certificate-chain validation.

### Fix

Windy resolved normally through the device's DNS, but the older ESP32 trust-bundle verifier returned HTTP -1 / TLS -12288 on its cross-signed Google certificate chain. The uploader now trusts Google's verified public GTS Root R1 directly. Certificate signatures, validity and hostname checks remain enabled; there is no insecure fallback.

The real device subsequently received HTTP 200 for a fresh live Atlas observation, and the owner confirmed their Windy station online. Credentials and Pi-hole settings were unchanged.

### Diagnostics

Protected Connection health now exposes the Windy last attempt, HTTP code, negative TLS error code, resolved address and DNS server. Successful handshakes show zero TLS error. Secrets and request headers are never exposed. Redact local network addresses when sharing diagnostic reports.

Fresh-data checks, five-minute successful cadence, rate-limit handling and bounded retry backoff remain unchanged. Other services retain their existing trust setup.

### Update and validation

Use **Updates → Check now** to install 0.8.1 from 0.8.0 or 0.8.1-rc.1. The owner's working candidate was left running for an OTA update. No 0.8.0 release asset was overwritten.

SC05_X build and host checks pass. Root fingerprint was verified against Google's PEM and the PC trusted store; live ESP32 acceptance and owner-confirmed Windy online status passed. Stable also normalizes the SDK's positive successful handshake return values in diagnostics.

Target: **PanelLan ZX2D80CE02S-V10 / SC05_X, ESP32-S3 N8R2** with documented CC1101 wiring. Atlas remains the operational integration. Application-only image: no credentials, captures, NVS, bootloader or partition table. ZIP includes notices; source is GPL-3.0-only.
