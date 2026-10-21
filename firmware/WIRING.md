# PanelLan SC05_X → pictured CC1101 module

Profile: `ZX2D80CE02S-V10` / `BOARD_SC05_X`, 433 MHz CC1101 board with
2 × 4 header and SMA antenna connector. This map uses the owner's
[`cc1101_pinout.webp`](../cc1101_pinout.webp), added October 20, 2026.
Firmware pin assignments are compiled into `platformio.ini` and
`include/radio_receiver.h`. Physical reception has not yet been bench verified.

## Connections

With the CC1101 oriented **as in the picture** (components facing you, SMA
connector to the right, header on the left):

| Header row | Left pin → connection | Right pin → connection |
| --- | --- | --- |
| Top | 1 GND → PanelLan **debug pin 7 GND** | 2 VCC → PanelLan **debug pin 2 +3.3 V** |
| Second | 3 GDO0 → GPIO14, expansion pin 7 | 4 CSN → GPIO13, expansion pin 6 |
| Third | 5 SCK → GPIO10, expansion pin 3 | 6 MOSI → GPIO12, expansion pin 5 |
| Bottom | 7 MISO/GDO1 → GPIO11, expansion pin 4 | 8 GDO2 → GPIO21, expansion pin 8 |

The PanelLan expansion port is an **8-pin MX1.25** connector, numbered by
its manufacturer. Do not infer connector orientation from the CC1101 image;
identify PanelLan pin 1 from the board marking/manufacturer drawing before
connecting. All GPIO signals are 3.3 V logic. The radio uses a separate SPI
bus from the parallel LCD and I²C touch interface. GDO0 is the receiver's
asynchronous data input to the ESP32; MISO/GDO1 remains the SPI data output.

Power uses the **7-pin debug header**; the six signals use the separate
**8-pin expansion header**. No wire in this selected pinout connects to
expansion pin 1 (+5 V). Identify the numbering on each connector from the
manufacturer drawing; do not count pins by copying the CC1101 orientation.

| CC1101 pin | Signal | PanelLan header | PanelLan pin / GPIO |
| --- | --- | --- | --- |
| 1 | GND | Debug, 7-pin | **7 · GND** |
| 2 | VCC | Debug, 7-pin | **2 · +3.3 V** |
| 3 | GDO0 | Expansion, 8-pin | 7 · GPIO14 |
| 4 | CSN | Expansion, 8-pin | 6 · GPIO13 |
| 5 | SCK | Expansion, 8-pin | 3 · GPIO10 |
| 6 | MOSI | Expansion, 8-pin | 5 · GPIO12 |
| 7 | MISO/GDO1 | Expansion, 8-pin | 4 · GPIO11 |
| 8 | GDO2 | Expansion, 8-pin | 8 · GPIO21 |

## Power from the PanelLan

The expansion connector's **pin 1 is 5 V**. Never connect that directly to
this CC1101 module's VCC. The [CC1101 datasheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf)
specifies 1.8–3.6 V operation; use a regulated 3.3 V supply.
Local manufacturer PDFs are in [reference/](../reference/README.md).

The selected direct-debug supply is described below. An alternative, if it
proves unsuitable, is to wire expansion pin 1 (5 V) to the input of a
suitable 3.3 V regulator, its output to CC1101 VCC, and its ground to
expansion pin 2 / CC1101 GND. Select a regulator rated comfortably above the
receiver's approximately 15–20 mA consumption, with the input/output
capacitors required by that regulator's datasheet. A salvaged regulator is
usable only after identifying its part number, pinout, input range, and
capacitor requirements. A resistor divider is not a regulator.

The PanelLan debug connector exposes **3.3 V on pin 2** and ground on pin 7.
The [manufacturer datasheet](https://img03.71360.com/w3/77q677/20250701/255a7c29ee6c23af4b5a41406aa43ffc.pdf)
labels the 3.3 V pin “For reference use, not for power input” and does not
specify an available output current. On October 20 the owner selected this
as a reasonable direct-power bench option: **debug pin 2 → CC1101 pin 2 VCC,
debug pin 7 → CC1101 pin 1 GND**. The signal pins remain on the expansion
connector as listed above. This is a proposed wiring option, not yet a
measured current-capacity or loaded-voltage result.

TI's Table 4 lists typical 433 MHz chip receive currents around 15–17 mA for
current-optimized configurations; sensitivity-optimized settings can draw
more. Allow approximately 15–20 mA as a typical receiver estimate, not a
guaranteed maximum for an unidentified module. Before connecting, verify the
pin voltage and orientation. Then check that it remains near 3.3 V with the
radio receiving while the display and Wi-Fi are active; check for resets or
reception problems. A separate regulator from the 5 V rail remains the
alternative if the debug supply proves unsuitable. Do not feed an external
3.3 V supply into the debug pin or back-power the ESP32.

Use short wiring, an appropriate 433 MHz antenna, and local radio decoupling
(typically 100 nF plus a few µF, in addition to the regulator's required
capacitors). With power disconnected, check continuity and pin numbering.
Measure the chosen supply output before attaching the radio.

## First live test

1. Power down, connect the eight signals/power pins above, and attach antenna.
2. Power up. `RF+` means SPI initialization succeeded; it does not prove RF
   reception. `RF-` leaves the display and setup page usable.
3. Tap the display title to find the setup network, access key, and URL.
   Confirm Atlas sensor ID `679`, channel `A`, and receiver enabled.
4. Watch packet count and per-field ages. Atlas types 37/38/39 update different
   fields; repeated RF copies are deduplicated. Five-minute gust coverage
   needs five continuous minutes before it is complete.
5. Compare readings and reception against the existing RTL-SDR reference.
   The PC capture service stays stopped unless explicitly restarted.

The board's RS485 connector is not an RS232 connection for a Kenwood D710.
No D710 wiring is authorized by this map.
