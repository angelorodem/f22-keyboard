# F22 RP2354 Split ZMK Firmware

This repository is a ZMK user-config repository and Zephyr board module for a wired split keyboard using an RP2354B on each half.

The build uses current upstream ZMK because RP2350/RP2350B support and Zephyr hardware model v2 board variants are needed for the targets below.

## Hardware Assumptions

- MCU: RP2354B on each half
- SoC target: `rp2350b/m33`
- Left half: USB/powered ZMK central
- Right half: UART split peripheral
- Caps Lock LED: GPIO31
- Wired split UART: TX GPIO0, RX GPIO1, UART0
- Matrix per half: 5 rows x 8 columns
- Matrix diode direction: row to column, ZMK `row2col`

Rows:

- ROW_0: GPIO4
- ROW_1: GPIO5
- ROW_2: GPIO6
- ROW_3: GPIO7
- ROW_4: GPIO8

Columns:

- COL_0: GPIO9
- COL_1: GPIO10
- COL_2: GPIO11
- COL_3: GPIO12
- COL_4: GPIO13
- COL_5: GPIO14
- COL_6: GPIO15
- COL_7: GPIO16

## Build Targets

GitHub Actions builds both UF2 files:

- `rp2354_split_left/rp2350b/m33/zmk`
- `rp2354_split_right/rp2350b/m33/zmk`

The build matrix is in [build.yaml](build.yaml). The firmware artifact is named `firmware`.

## Editing The Keymap

The default keymap is in [config/rp2354_split.keymap](config/rp2354_split.keymap). It keeps the Advantage 360 logical layout and layer structure, with Kinesis-specific RGB, BLE, backlight, battery, and Studio bindings replaced by inert bindings for a plain RP2354 wired split build.

The keymap-editor files are included:

- [config/keymap.json](config/keymap.json)
- [config/info.json](config/info.json)

Use [Nick Coutsos's ZMK keymap editor](https://nickcoutsos.github.io/keymap-editor/) with this repository.

## Caps Lock LED

GPIO31 is configured as a HID Caps Lock indicator. When the host reports Caps Lock as active, the GPIO31 LED turns on. When Caps Lock is inactive, it turns off. HID indicator state is also forwarded to the wired split peripheral, so the right half can mirror the indicator if it has the same LED wiring.

## Flashing

ZMK cannot flash the passive half over TRRS/UART. Flash both MCUs manually:

1. Hold BOOTSEL on the left half and plug in USB.
2. Copy the left UF2 from the GitHub Actions artifact.
3. Hold BOOTSEL on the right half and plug in USB.
4. Copy the right UF2 from the artifact.
5. Unplug USB.
6. Connect the split cable.
7. Plug USB into the left half.

Do not connect or disconnect the split cable while either half is powered.

## Local Build

After setting up a ZMK west workspace, build from the workspace root with:

```sh
west build -s zmk/app -d build/left -b rp2354_split_left/rp2350b/m33/zmk -S rp2-boot-mode-retention -- -DZMK_CONFIG="$PWD/config"
west build -s zmk/app -d build/right -b rp2354_split_right/rp2350b/m33/zmk -S rp2-boot-mode-retention -- -DZMK_CONFIG="$PWD/config"
```
