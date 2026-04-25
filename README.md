# F22 RP2354 Split ZMK Firmware

This repository is a ZMK user-config repository and Zephyr board module for a wired split keyboard using an RP2354B on each half.

The build uses current upstream ZMK and includes a small RP2350B compatibility shim for the Zephyr 4.1 hardware model used by ZMK.

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

GitHub Actions builds normal and debug UF2 files:

- `rp2354_split_left_plain`
- `rp2354_split_left_boot_led`
- `rp2354_split_left/rp2350b/m33/zmk`
- `rp2354_split_right/rp2350b/m33/zmk`

The build matrix is in [build.yaml](build.yaml). The firmware artifact is named `firmware`.

For USB bring-up debugging, GitHub Actions also builds a `rp2354_split_left_usb_logging` UF2 that exposes a USB CDC ACM log port on the left half.

## Editing The Keymap

The default keymap is in [config/rp2354_split.keymap](config/rp2354_split.keymap). It keeps the Advantage 360 logical layout and layer structure, with Kinesis-specific RGB, BLE, backlight, battery, and Studio bindings replaced by inert bindings for a plain RP2354 wired split build.

The keymap-editor files are included:

- [config/keymap.json](config/keymap.json)
- [config/info.json](config/info.json)

Use [Nick Coutsos's ZMK keymap editor](https://nickcoutsos.github.io/keymap-editor/) with this repository.

## Caps Lock LED

GPIO31 is configured as a HID Caps Lock indicator on the left half. When the host reports Caps Lock as active, the GPIO31 LED turns on. When Caps Lock is inactive, it turns off.

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

For first bring-up and USB debugging:

1. Flash `rp2354_split_left_plain.uf2` to the left half.
2. Leave the right half disconnected for the first USB test.
3. Plug USB into the left half and check Windows Device Manager for a new keyboard/HID device.
4. If `rp2354_split_left_plain.uf2` enumerates, the issue is in an added snippet rather than the base board bring-up.
5. If it does not enumerate, flash `rp2354_split_left_boot_led.uf2` and check whether GPIO31 turns on.
6. If the boot LED turns on, the firmware is booting far enough to initialize GPIO and the next issue is USB device stack bring-up.
7. If the boot LED does not turn on, the firmware may not be booting or GPIO31 polarity/wiring may not match the board.
8. Flash `rp2354_split_left_usb_logging.uf2` and check Windows Device Manager for a new USB serial device.
9. If the left half enumerates with either USB debug image, flash the normal left UF2 again, then flash the right UF2 and test the full split with the UART cable attached before power-up.

## Local Build

After setting up a ZMK west workspace, build from the workspace root with:

```sh
west build -s zmk/app -d build/left -b rp2354_split_left/rp2350b/m33/zmk -S rp2-boot-mode-retention -- -DZMK_CONFIG="$PWD/config" -DZMK_EXTRA_MODULES="$PWD"
west build -s zmk/app -d build/right -b rp2354_split_right/rp2350b/m33/zmk -S rp2-boot-mode-retention -- -DZMK_CONFIG="$PWD/config" -DZMK_EXTRA_MODULES="$PWD"
```
