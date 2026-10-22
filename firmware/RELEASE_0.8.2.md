WX Link v0.8.2 fixes an OTA startup-check deadlock introduced with the 0.8.0 settings validation probes.

The pending-boot save lock returned HTTP 409 before semantic validation could reject deliberately invalid settings with HTTP 400. Startup checks failed and rollback restored the previous working image. Settings validation now completes before the save lock; valid requests still cannot save during startup or an update. No credentials, history or update integrity safeguards are changed.

Startup checks now explicitly verify both invalid-setting rejection and a valid request being blocked while boot confirmation is pending. Factory-reset probes remain non-destructive.

The Windy TLS fix from 0.8.1 is retained. Stable/Dev update-channel selection is documented for a future release only.

Validation: SC05_X firmware build and complete host-check script pass. The owner device installed the public image through OTA, passed startup checks, confirmed at 33 seconds and remained running beyond 100 seconds without rollback. Interrupted-transfer and deliberate failed-boot testing remain open.
