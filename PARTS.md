# Low-cost new-parts proposal

Planning estimate, October 15, 2026. Prices below are USD shopping targets, not verified checkout quotes; exclude tax/shipping. This is one controller in one enclosure, with wired peripherals, not a network gateway architecture.

## Owned display/controller candidate: PanelLan ZX2D80CE02S-V10

Owner identified the connected board on October 20, 2026 as
`PanelLan ZX2D80CE02S-V10`. The manufacturer's
[Arduino library](https://github.com/smartpanle/PanelLan_esp32_arduino)
maps `ZX2D80CE02S` to `SC05_X` / `BOARD_SC05_X`: ESP32-S3, 8 MB QIO flash,
QSPI PSRAM, ST7789 display on an 8-bit 8080 parallel bus, and FT5x06 touch.
The [matching manufacturer datasheet](https://img03.71360.com/w3/77q677/20250701/255a7c29ee6c23af4b5a41406aa43ffc.pdf)
lists a 2.8-inch, 240×320 panel and `WT32-S3-WROVER-N8R2` module, indicating
2 MB PSRAM. The October 20 chip probe independently confirmed ESP32-S3 revision 0.1,
8 MB flash, and 2 MB embedded PSRAM on the owner's unit. Landscape rotation would provide the planned 320×240 view.

The datasheet lists six expansion signals, GPIO 10, 11, 12, 13, 14, and 21,
on an eight-pin MX1.25 connector alongside 5 V and ground; GPIO logic is 3.3 V.
It separately lists UART0/EN/BOOT on a seven-pin debug connector and an RS485
interface. RS485 is not the D710 serial electrical interface. Confirm connector
orientation, actual revision, schematic, and access to shared I2C before
assigning peripherals. The owner removed SD logging as a requirement on
October 20, 2026; no card or SD pins are needed for the baseline. The referenced
library does not list onboard SD for this model.

PC USB identification was `303a:1001` (Espressif USB JTAG/serial), `/dev/ttyACM0`,
serial `68:B6:B3:34:82:48`. Serial read/write access was verified on October 20, 2026 after installing
the device-specific permission rule below; the port opened successfully
without sending data.
Chip identification and a private full-flash backup are complete. Initial
firmware and measured display/touch validation are documented in
[firmware/README.md](firmware/README.md) and
[firmware/VALIDATION.md](firmware/VALIDATION.md). Peripheral wiring remains unverified. This integrated board is an owned prototype candidate,
separate from the new-purchase baseline below; no new board/display purchase
is implied. Feasibility with all required peripherals remains to be validated.

### PC serial permissions

The device-specific rule in `tools/70-cyd-weather-link.rules` matches the owned
board's USB vendor/product and serial number, granting active desktop users
serial access through the standard `uaccess` tag. The installer also applies
an immediate named ACL for `lschaar` on the connected device. It does not grant
access to unrelated serial devices. To
install and apply it from a terminal in this project directory:

```bash
sudo bash tools/setup-esp32-access.sh
```

This requires the owner's sudo password in their own terminal. The setup script
installs the rule in `/etc/udev/rules.d/`, reloads rules, and reapplies them to
the matching connected tty. Reconnection will also apply the persistent rule.
A newly acquired board with a different serial requires updating the rule.
This configures host permissions only; it does not flash or reset firmware.

## Recommended baseline for a new purchase

| Part | Specification | Target allowance |
| --- | --- | --- |
| ESP32-S3 development board | WROOM-1 N16R8 preferred; N8R8 acceptable; exposed headers, onboard Wi-Fi antenna | $6–12 |
| SPI touchscreen | 2.8 inch, 320×240, ILI9341, documented touch controller; SD socket optional | $7–12 |
| Receiver | 433 MHz CC1101 SPI module, accessible GDO0/GDO2, antenna included | $3–6 |
| Barometer | BMP280 I2C breakout, 3.3 V compatible | $1–3 |
| Serial interface, optional addition | 3.3 V MAX3232-class breakout, subject to exact D710 electrical requirements | $1–3 |
| Wiring and mounting | Headers, perfboard, short wiring, decoupling | $3–5 |

Estimated electronics total without optional SD: $21–41; budget about $30–40 with sensible sourcing and reused power supply/enclosure. Add allowance for a USB power supply, enclosure, and an optional D710 cable if that output is selected. Verify listings against these specifications before ordering.

Use N16R8 for 16 MB flash and 8 MB PSRAM if its premium is small. Extra flash helps accommodate two firmware images for updates; PSRAM provides buffer headroom but does not replace internal RAM needed for some real-time and DMA operations.

The proposed swipe pages, rolling 24-hour history, and local internet forecast
in [SCREENS.md](SCREENS.md) use the existing touchscreen, Wi-Fi, and
bounded RAM/PSRAM and internal flash. SD is optional; the card allowance was
removed after the October 20 requirement change. These pages add no component
or price allowance to this baseline. Verify
drag/swipe behavior with the exact touch controller; budget chart/history and
forecast/TLS buffers alongside the display, radio, and history tasks. The preferred
PSRAM profile remains proposed until measured on the actual board; these
features do not establish compatibility with every CYD variant.

Prefer a separate display module plus development board for the first reproducible hardware profile. This exposes usable GPIO and allows us to allocate buses explicitly. Reserve one SPI controller for CC1101 and another for display/touch where supported by the selected drivers. I2C pressure and a dedicated weather-output UART remain separate. Confirm all pin assignments against the actual board and flash/PSRAM connections.

A classic CYD remains a potential supported profile, but its limited expansion pins make it a less attractive new purchase with receiver, pressure, and UART sharing limited exposed pins. An integrated ESP32-S3 display board may reduce wiring, but must have a verified complete pin budget before recommendation.

Do not substitute a UART transparent-data radio for a CC1101 SPI module. The receiver needs direct radio configuration and pulse/interrupt access for Atlas decoding. Firmware and receiver range still require prototype validation.

## Weather-sharing interfaces

Internet weather sharing to APRS/CWOP, Weather Underground, Weathercloud, Windy,
and later PWSWeather uses the existing Wi-Fi hardware; see
[WEATHER_SHARING.md](WEATHER_SHARING.md).
No additional part or price allowance is introduced. Meshtastic was reviewed
as a possible later bridge, but compatible radio hardware and an interface are
not selected or included in this baseline. Preserve the single-controller
architecture and CC1101/display/serial pin budget while evaluating any future addition.

## Optional sound alerts

Sound is an optional addition to the baseline total above. First identify the exact display board and check whether it already has a usable amplifier/speaker connection; do not assume all CYDs do.

For the separate ESP32-S3 profile, use a **MAX98357A I²S amplifier breakout** and a matched small speaker (for example, an 8 Ω speaker with a power rating suitable for the chosen supply and volume). The [manufacturer](https://www.analog.com/en/products/max98357a.html) specifies 2.5–5.5 V operation and no MCLK requirement. Reserve three GPIO for BCLK/LRCLK/DIN, plus an optional enable control; verify supply, logic compatibility, speaker load, decoupling, and pin assignments against the exact breakout. Connect the speaker across the amplifier's speaker outputs as documented, not to GPIO or ground. Budget power-supply headroom and enclosure space; verify audibility and radio interference in the assembled appliance.

A piezo buzzer with a suitable driver is an alternative for basic beeps. Choose one audio path after the pin budget is verified. No verified purchase price or revised total is available yet. Alert chimes, volume, mute, test, and acknowledgement behavior are specified in [HOME_SCREEN.md](HOME_SCREEN.md).

## References checked

- [Waveshare ESP32-S3 development boards](https://www.waveshare.com/product/esp32-s3-dev-kit-n8r8.htm): vendor lists $9.99–15.99 across variants; lower table targets assume budget marketplace sourcing.
- [Espressif DevKitC documentation](https://documentation.espressif.com/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html): memory variants and header assignments.
- [Documented ILI9341 SPI touchscreen/SD module](https://www.lcdwiki.com/2.8inch_IPS_SPI_Module_ILI9341): example MSP2834 has capacitive touch and shared LCD/SD SPI lines; match exact variant to the eventual firmware profile.
- [Ebyte E07-M1101D-SMA](https://www.ebyte.com/product/57.html): example 433 MHz CC1101 SPI radio family.
- [rtl_433_ESP](https://github.com/NorthernMan54/rtl_433_ESP): initial receiver library candidate; validate the selected ESP32-S3 build and Atlas packets.
- [MAX98357A manufacturer documentation](https://www.analog.com/en/products/max98357a.html): candidate digital-input speaker amplifier; audio remains optional and unbenchmarked.

## Implemented CC1101 pin allocation

The firmware now assigns the six external GPIOs for CC1101: SCK10, MISO11,
MOSI12, CS13, GDO0=14 and GDO2=21. The exact connector pins, the owner's
8-pin module photo and regulated 3.3 V power options are in
[firmware/WIRING.md](firmware/WIRING.md). Expansion power pin 1 is 5 V.
The debug connector's pin 2 provides 3.3 V, with pin 7 ground; available
output current remains unspecified. The owner selected it as a direct-power
bench option, pending voltage checks with the receiver, display and Wi-Fi
running. A small 3.3 V regulator from the board's 5 V rail remains an
alternative. See the wiring guide for the distinction between this proposal
and bench-verified operation. This pin allocation implements the single-controller design without
an SD requirement. It uses every external GPIO; adding a barometer or D710
requires a separate verified interface allocation, not an assumed free pin.


### Seven-day recording storage update

The owned 8 MB PanelLan profile retains its 1.5 MiB internal data partition and
both OTA slots. The seven-day/five-minute archive adds 126 KiB of logical
record storage plus filesystem overhead, with a 512 KiB free-space reserve.
No SD card or additional controller is required. See the matching requirement
in DESIGN.md and [device recording](firmware/RECORDING.md).


The October 21 BARO display fallback uses nearby NWS station observations while
no local pressure sensor is fitted. This adds no hardware and is not a local
barometric measurement; see DESIGN.md and HOME_SCREEN.md for freshness/source
labeling. A local barometer remains an optional addition.
