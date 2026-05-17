# Split Keyboards

ZMK supports keyboards split into two or more physical parts, each with its own controller running ZMK. The parts communicate so the host sees one keyboard.

## Central and Peripheral Roles

- One part is the central: it receives key/sensor events from peripherals, runs the keymap, converts events to HID output, and talks to the host over USB or Bluetooth.
- Other parts are peripherals: they only talk to the central.
- Internal state such as active layers lives only on the central.
- Peripherals do not present as USB keyboards and do not advertise as pairable BLE keyboards.
- If the keyboard uses a dongle, the dongle becomes the central.
- By convention on 2-piece boards, the left half is central and the right half is peripheral.

## Battery Life Impact

For BLE splits, the central uses significantly more power because its radio must wake periodically to check for incoming traffic. Use the power profiler for role-specific battery-life estimates.

## Configuration

The new shield guide covers defining a split shield, enabling split support, and assigning roles. Relevant options include:

- CONFIG_ZMK_SPLIT: enables split support.
- CONFIG_ZMK_SPLIT_ROLE_CENTRAL: marks the build as the central side.
- CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS: number of peripherals connected to the central.

## Latency

Peripheral-originated events have higher latency because they pass through the central. With the current BLE transport, split adds about 3.75 ms average latency and up to 7.5 ms worst case.

## Split Transports

ZMK supports two split transports:

- Bluetooth
- Full-duplex wired UART

Only one transport can be active at a time. Hybrid designs using both simultaneously are not supported.

## Hot Plugging Cables

### Cable Warning

Many cables, especially TRRS/TRS, can permanently damage controllers if inserted or removed while power is present. Never hot-plug them while a controller is powered by USB or battery, even if you are not using wired split.

## Bluetooth

- This is the most tested and most flexible split transport.
- A central can connect to multiple peripherals.
- This enables dongle-based setups and multi-part keyboards.
- It is enabled when CONFIG_ZMK_SPLIT=y and CONFIG_ZMK_BLE=y on a supported MCU/controller.

## Full-Duplex Wired (UART)

- Intended for early adopters and newer than BLE split.
- Supports one central and one peripheral.
- Unlike BLE, the central cannot currently connect to more than one wired peripheral.
- It is enabled when CONFIG_ZMK_SPLIT=y and the devicetree contains a node with compatible = "zmk,wired-split".

## Full Duplex vs Half Duplex

- Full-duplex UART needs two wires between halves.
- Planned half-duplex support will use one bidirectional wire.
- Until half-duplex exists, designs such as Corne and Sweep that use a single GPIO for inter-half communication cannot use wired split and must use Bluetooth.

## Runtime Switching

### Runtime Switching Warning

Transport switching is highly experimental and requires hardware designed specifically for it. Using it on boards not designed for runtime transport switching, such as Corne or Sofle, can cause permanent damage.

There are currently no open-source/reference designs for this. Only experienced designers with strong EE knowledge should attempt it.

## Building and Flashing Firmware

- Split keyboards require separate firmware builds for each part.
- In typical GitHub workflow builds for 2-part splits, outputs usually include left and right firmware files.
- Flash each file to its matching half.

## Updating Your Keymap

- Keymap processing happens mainly on the central, so keymap-only changes usually require reflashing only the central.
- If you change shared config files, flash all parts.
- Some ZMK changes related to split features may also require reflashing all parts.

## Pairing for Wireless Split Keyboards

- BLE-based split keyboards use an internal pairing process between central and peripherals.
- When the central has an open peripheral slot, it advertises for connections invisible to non-ZMK devices.
- Any unbonded peripheral will pair to it.
- Bonding data is stored on both sides with the other part's hardware address, similar to host Bluetooth profiles.
- In normal use, the parts pair automatically the first time they are powered together.
- If pairing fails, or the controllers were used in another split setup, clear stored bond information and re-pair using the troubleshooting procedure.

### Pairing Warning

If the central is advertising for pairing or waiting for disconnected peripherals, power consumption increases and batteries drain faster.

## Behaviors with Locality

Most behaviors are processed only on the central because it owns the keymap and host communication. Some behaviors have locality:

### Global Locality Behaviors

These affect all keyboard parts:

- RGB underglow behaviors
- Backlight behaviors
- Power management behaviors
- Soft off behavior

### Source Locality Behaviors

These affect only the keyboard part that invoked them:

- Reset behaviors

### Peripheral Invocation

Peripherals must be paired and connected before they can trigger even local-peripheral effects, because the binding is still processed on the central, which then instructs the peripheral to run the effect.

## Combos

Combos always invoke source-locality behaviors on the central.

## Debouncing

ZMK uses a cycle-based debounce algorithm with independent debouncing per key to filter contact bounce and noise.

- Default press debounce: 5 ms
- Default release debounce: 5 ms

Lower values reduce latency; higher values improve robustness. If one press produces multiple inputs, increase press and/or release debounce and also inspect for mechanical issues such as poor hot-swap socket contact.

## Debounce Configuration

### Debounce Note

These options are supported by zmk,kscan-gpio-matrix and zmk,kscan-gpio-direct. zmk,kscan-gpio-demux does not support them.

### Global Options

Set these in a .conf file. Values must be <= 16383.

- CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS: key press debounce, default 5.
- CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS: key release debounce, default 5.

If set, a global option overrides the matching per-driver option.

Example:

```conf
CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=3
CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS=3
```

### Per-Driver Options

Set these on a kscan devicetree node. Values must be <= 16383.

- debounce-press-ms: default 5.
- debounce-release-ms: default 5.
- debounce-period: deprecated; sets both press and release.
- debounce-scan-period-ms: scan interval while any key is pressed; default 1.

If the matching global option is set, it overrides the per-driver property.

Example, assuming the driver is labeled kscan0:

```dts
kscan0: kscan {
    compatible = "zmk,kscan-gpio-matrix";
    ...
};

&kscan0 {
    debounce-press-ms = <3>;
    debounce-release-ms = <3>;
};
```

The override must be placed outside any other node block.

`debounce-scan-period-ms` controls scan frequency during debouncing. Larger values reduce power use, but press/release debounce values are rounded up to the next multiple of the scan period. Example: a 2 ms scan period with 5 ms debounce results in 6 ms effective debounce.

## Eager Debouncing

Eager debouncing reports a key change immediately, then ignores further changes for the debounce window. It minimizes latency but is less resistant to noise.

ZMK does not support true eager debouncing, but you can approximate it by using zero press debounce and a larger release debounce:

```conf
CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=0
CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS=5
```

Using CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=1 is often a safer compromise because it adds only 1 ms latency while filtering short spikes.

## Comparison With QMK

- ZMK default debouncing is similar to QMK sym_defer_pk.
- CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=0 approximates QMK asym_eager_defer_pk.

See the QMK Debounce API documentation for details.

## Layout Configuration

See Configuration Overview for how to apply these settings.

## Matrix Transform

A matrix transform maps logical keymap positions to physical kscan positions.

- You can define multiple transforms, one per layout.
- Users select a transform through the /chosen node in their keymap.
- See the new shield guide for full setup details.

### Devicetree

Applies to: compatible = "zmk,matrix-transform"

Definition file: zmk/app/dts/bindings/zmk,matrix-transform.yaml

| Property | Type | Description | Default |
| --- | --- | --- | --- |
| rows | int | Number of rows in the transformed matrix | |
| columns | int | Number of columns in the transformed matrix | |
| row-offset | int | Added to all rows before lookup | 0 |
| col-offset | int | Added to all columns before lookup | 0 |
| map | array | List of position transforms | |

Define `map` using the RC() macro from dt-bindings/zmk/matrix_transform.h. It has one item per logical key position, and each item specifies the physical row and column that trigger that key.

### Example: Skipping Unused Positions

Use a transform to omit unused physical matrix positions so users do not need to place `&none` in the keymap.

```dts
// numpad.overlay
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix-transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        // define row-gpios with 5 elements and col-gpios with 4...
    };

    default_transform: matrix_transform {
        compatible = "zmk,matrix-transform";
        rows = <5>;
        columns = <4>;
        // ┌───┬───┬───┬───┐
        // │NUM│ / │ * │ - │
        // ├───┼───┼───┼───┤
        // │ 7 │ 8 │ 9 │ + │
        // ├───┼───┼───┤   │
        // │ 4 │ 5 │ 6 │   │
        // ├───┼───┼───┼───┤
        // │ 1 │ 2 │ 3 │RET│
        // ├───┴───┼───┤   │
        // │ 0     │ . │   │
        // └───────┴───┴───┘
        map = <
            RC(0,0) RC(0,1) RC(0,2) RC(0,3)
            RC(1,0) RC(1,1) RC(1,2) RC(1,3)
            RC(2,0) RC(2,1) RC(2,2)
            RC(3,0) RC(3,1) RC(3,2) RC(3,3)
            RC(4,0)         RC(4,1)
        >;
    };
};

// numpad.keymap
/ {
    keymap {
        compatible = "zmk,keymap";
        default {
            bindings = <
                &kp KP_NUM &kp KP_DIV &kp KP_MULT &kp KP_MINUS
                &kp KP_N7  &kp KP_N8  &kp KP_N9   &kp KP_PLUS
                &kp KP_N4  &kp KP_N5  &kp KP_N6
                &kp KP_N1  &kp KP_N2  &kp KP_N3   &kp KP_ENTER
                &kp KP_N0             &kp KP_DOT
            >;
        };
    };
};
```

### Example: Non-Standard Matrix

For a duplex matrix with twice as many rows and half as many columns as keys, use a transform so the keymap matches the physical layout instead of the raw matrix.

```dts
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix-transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
        // define row-gpios with 12 elements and col-gpios with 8...
    };

    default_transform: matrix_transform {
        compatible = "zmk,matrix-transform";
        rows = <6>;
        columns = <16>;
        // ESC F1 F2 F3   ...
        // `   1  2  3    ...
        // Tab  Q  W  E   ...
        // Caps  A  S  D  ...
        // Shift  Z  X  C ...
        // Ctrl Alt       ...
        map = <
            RC(0,0) RC(1,0) RC(0,1) RC(1,1)       // ...
            RC(2,0) RC(3,0) RC(2,1) RC(3,1)       // ...
            RC(4,0)   RC(5,0) RC(4,1) RC(5,1)     // ...
            RC(6,0)     RC(7,0) RC(6,1) RC(7,1)   // ...
            RC(8,0)       RC(9,0) RC(8,1) RC(9,1) // ...
            RC(10,0) RC(11,0)                     // ...
        >;
    };
};
```

### Example: Charlieplex

Charlieplexed layouts always need a matrix transform because addressable positions do not map directly to the keyboard layout. You only need to map the positions you actually use.

```dts
/ {
    chosen {
        zmk,kscan = &kscan0;
        zmk,matrix-transform = &default_transform;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-charlieplex";
        wakeup-source;

        interrupt-gpios = <&pro_micro 21 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;
        gpios
          = <&pro_micro 16 GPIO_ACTIVE_HIGH>
          , <&pro_micro 17 GPIO_ACTIVE_HIGH>
          , <&pro_micro 18 GPIO_ACTIVE_HIGH>
          , <&pro_micro 19 GPIO_ACTIVE_HIGH>
          , <&pro_micro 20 GPIO_ACTIVE_HIGH>
          ; // addressable space is 5x5, minus paired values
    };

    default_transform: matrix_transform {
        compatible = "zmk,matrix-transform";
        rows = <3>;
        columns = <5>;
        //  Q  W  E  R
        //   A  S  D  F
        //    Z  X  C  V
        map = <
            RC(0,1) RC(0,2) RC(0,3) RC(0,4)
              RC(1,0) RC(1,2) RC(1,3) RC(1,4)
                RC(2,0) RC(2,1) RC(2,3) RC(2,4)
        >;
    };
};
```

## Physical Layout

A physical layout combines a matrix transform, a kscan, and key physical attributes. Multiple physical layouts are allowed for keyboards with multiple physical arrangements.

### Physical Layout Devicetree

Applies to: compatible = zmk,physical-layout

Definition file: zmk/app/dts/bindings/zmk,physical-layout.yaml

| Property | Type | Description | Default |
| --- | --- | --- | --- |
| display-name | string | Display name for the layout | |
| transform | phandle | Matrix transform used by this layout | |
| kscan | phandle | Kscan used by this layout; falls back to chosen zmk,kscan if omitted | |
| keys | phandle-array | Array of key physical attributes | |

Each `keys` entry has the form `<&key_physical_attrs w h x y r rx ry>`.

| Field | Type | Description | Unit |
| --- | --- | --- | --- |
| Width | int (>0) | Key width | centi-keyunit |
| Height | int (>0) | Key height | centi-keyunit |
| X | uint | Key X position, top-left | centi-keyunit |
| Y | uint | Key Y position, top-left | centi-keyunit |
| Rotation | int | Key rotation, positive clockwise | centi-degree |
| Rotation X | int | Rotation origin X | centi-keyunit |
| Rotation Y | int | Rotation origin Y | centi-keyunit |

The `key_physical_attrs` node, defined in `dts/physical_layouts.dtsi`, is mandatory.

### Physical Layout Kconfig

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_PHYSICAL_LAYOUT_KEY_ROTATION | bool | Store/support key rotation internally | y |

## Physical Layout Position Map

This maps positions across physical layouts so ZMK Studio can preserve key assignments between layouts.

### Physical Layout Position Map Devicetree

Applies to: compatible = zmk,physical-layout-position-map

Definition file: zmk/app/dts/bindings/zmk,physical-layout-position-map.yaml

| Property | Type | Description | Default |
| --- | --- | --- | --- |
| complete | boolean | Mapping is complete and should replace position-based mapping | |

The node has one child per physical layout.

| Child Property | Type | Description | Default |
| --- | --- | --- | --- |
| physical-layout | phandle | Physical layout for this mapping entry | |
| positions | array | Positions aligned by index with sibling nodes | |

## Configuration Overview

ZMK configuration lives in Kconfig and Devicetree files.

### Configuration Note

Configuration is compile-time only. Any change requires rebuilding and reflashing firmware.

## Config File Locations

ZMK searches three main places for config files.

### User Config Folder

When building with a `zmk-config` folder, ZMK looks in `zmk-config/config` for:

- `<name>.conf` for Kconfig
- `<name>.keymap` for Devicetree

Here, `<name>` is the shield name when using a shield, otherwise the board name.

- These files contain personal settings.
- They are optional.
- If present, they override board and shield configuration.
- Otherwise the default config/keymap is used.

For split keyboards, a shared file without `_left` or `_right` applies to both halves. Example: `corne.conf` and `corne.keymap` apply to `corne_left` and `corne_right`. If a shared file exists, side-specific files are ignored.

### Board Folder

ZMK searches:

- `zmk/app/boards/arm/<board>`
- `zmk-config/boards/arm/<board>`
- `<module>/boards/arm/<board>`
- `zmk-config/config/boards/arm/<board>` for backward compatibility only; do not use it

Besides Zephyr board files, it looks for:

- `<board>.conf`
- `<board>.keymap` for keyboards with onboard controllers only

Shared no-suffix files are not supported in board folders. For more, see Zephyr's board porting guide.

### Shield Folder

ZMK searches:

- `zmk/app/boards/shields/<shield>`
- `zmk-config/boards/shields/<shield>`
- `<module>/boards/shields/<shield>`
- `zmk-config/config/boards/shields/<shield>` for backward compatibility only; do not use it

Besides Zephyr shield files, it looks for:

- `<shield>.conf`
- `<shield>.keymap`

Shared no-suffix files are not supported in shield folders. For more, see Zephyr shield documentation and the ZMK new keyboard shield guide.

## Kconfig Files

Kconfig configures global behavior such as keyboard name and hardware feature enablement. Files usually end in `.conf` and contain one `CONFIG_X=value` assignment per line.

Example:

```conf
CONFIG_ZMK_SLEEP=y
CONFIG_EC11=y
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
```

- Available options are defined by ZMK files whose names begin with `Kconfig`.
- Files ending in `_defconfig` use the same syntax for hardware-specific defaults that users typically do not need to change.
- In `_defconfig` files, options are not prefixed with `CONFIG_`.

### Kconfig Tip

Inspect the final resolved build configuration when troubleshooting Kconfig changes.

## Kconfig Value Types

- `bool`: `y` or `n`. Example: `CONFIG_FOO=y`
- `int`: integer value. Example: `CONFIG_FOO=42`
- `string`: quoted text. Example: `CONFIG_FOO="foo"`

## Devicetree Files

Devicetree files describe hardware and define keymaps. Multiple files are merged into one final tree.

Common extensions:

- `.dts`: base hardware definition
- `.overlay`: adds to or overrides a `.dts`
- `.keymap`: keymap and user hardware configuration
- `.dtsi`: include-only devicetree source

Example:

```dts
/ {
    chosen {
        zmk,kscan = &kscan0;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-matrix";
    };
};
```

- Properties are attached to specific nodes, not set globally.
- Allowed properties come from `.yaml` binding files in ZMK's `dts/bindings` directories.

### Devicetree Tip

Inspect the final merged Devicetree when troubleshooting node changes.

## Changing Devicetree Properties

To change a property, first find the node in the board `.dts`, shield `.overlay`, or any included `.dtsi`. Example node:

```dts
kscan0: kscan {
    compatible = "zmk,kscan-gpio-matrix";
    // more properties and/or nodes...
};
```

- The part before the colon, `kscan0`, is an optional label.
- The part after the colon, `kscan`, is the node name.
- The `compatible` property determines which binding defines supported properties.

To override an existing labeled node, reference it by label outside other nodes:

```dts
&kscan0 {
    debounce-press-ms = <0>;
};
```

If the node has no label, you can also write a matching tree fragment and let Devicetree merge it:

```dts
/ {
    kscan {
        debounce-press-ms = <0>;
    };
};
```

## Devicetree Property Types

- `bool`: present means true; absent means false. Example: `property;` To override true to false, use `/delete-property/ the-property-name;`
- `int`: single integer in angle brackets. Example: `property = <42>;`
- `string`: quoted text. Example: `property = "foo";`
- `array`: list of integers in angle brackets, space-separated. Expressions must be wrapped in parentheses. Example: `property = <1 2 3 4>;` Multiple blocks are allowed: `property = <1 2>, <3 4>;`
- `phandle`: single node reference in angle brackets. Example: `property = <&label>;`
- `phandles`: list of node references in angle brackets. Example: `property = <&label1 &label2 &label3>;`
- `phandle-array`: list of node references with optional numeric parameters. Example: `property = <&none &mo 1>;` Multiple blocks are allowed: `property = <&none>, <&mo 1>;`
- `GPIO array`: a `phandle-array` used for GPIOs. Each entry is a GPIO controller label, an index, and flags.
- `path`: node path as a label or string. Examples: `property = &label;` or `property = "/path/to/some/node";`

Example GPIO array:

```dts
some-gpios =
    <&gpio0 0 GPIO_ACTIVE_HIGH>,
    <&gpio0 1 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
    ;
```

## Keyboard Scan Configuration

See Configuration Overview for usage.

### Common

#### Common Kconfig

Definition files:

- `zmk/app/Kconfig`
- `zmk/app/module/drivers/kscan/Kconfig`

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_KSCAN_EVENT_QUEUE_SIZE | int | Kscan event queue size | 4 |
| CONFIG_ZMK_KSCAN_INIT_PRIORITY | int | Kscan driver init priority | 40 |
| CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS | int | Global press debounce in ms | -1 |
| CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS | int | Global release debounce in ms | -1 |

If either debounce value is not -1, it overrides matching `debounce-press-ms` or `debounce-release-ms` properties for supported drivers.

#### Common Devicetree

Applies to: `/chosen` node

| Property | Type | Description |
| --- | --- | --- |
| zmk,kscan | path | Keyboard scan driver node to use |
| zmk,matrix-transform | path | Matrix transform node to use |

### Demux Driver

Works like a regular matrix, but uses a demultiplexer to drive rows or columns, so N GPIOs can drive N^2 rows or columns instead of only N.

#### Demux Note

This driver does not honor `CONFIG_ZMK_KSCAN_DEBOUNCE_*` settings.

#### Demux Devicetree

Applies to: `compatible = "zmk,kscan-gpio-demux"`

Definition file: `zmk/app/module/dts/bindings/kscan/zmk,kscan-gpio-demux.yaml`

| Property | Type | Description | Default |
| --- | --- | --- | --- |
| input-gpios | GPIO array | Input GPIOs | |
| output-gpios | GPIO array | Demultiplexer address GPIOs | |
| debounce-period | int | Debounce period in ms | 5 |
| polling-interval-msec | int | Polling interval in ms | 25 |

### Direct GPIO Driver

Each key has its own GPIO.

#### Direct GPIO Kconfig

Definition file: `zmk/app/module/drivers/kscan/Kconfig`

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_KSCAN_DIRECT_POLLING | bool | Poll for key presses instead of using interrupts | n |

#### Direct GPIO Devicetree

Applies to: `compatible = "zmk,kscan-gpio-direct"`

Definition file: `zmk/app/module/dts/bindings/kscan/zmk,kscan-gpio-direct.yaml`

| Property | Type | Description | Default |
| --- | --- | --- | --- |
| input-gpios | GPIO array | Input GPIOs, one per key; may be direct pins or gpio-key refs | |
| debounce-press-ms | int | Press debounce in ms; use 0 for eager behavior | 5 |
| debounce-release-ms | int | Release debounce in ms | 5 |
| debounce-scan-period-ms | int | Scan interval while any key is pressed | 1 |
| poll-period-ms | int | Scan interval when idle and CONFIG_ZMK_KSCAN_DIRECT_POLLING is enabled | 10 |
| toggle-mode | bool | Enable toggle switch mode | n |
| wakeup-source | bool | Allow this kscan to wake the keyboard | n |

Assuming switches pull GPIOs to ground, `input-gpios` should use `GPIO_ACTIVE_LOW | GPIO_PULL_UP`:

```dts
kscan0: kscan {
    compatible = "zmk,kscan-gpio-direct";
    wakeup-source;
    input-gpios
        = <&pro_micro 4 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
        , <&pro_micro 5 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
        ;
};
```

A direct pin in `input-gpios` is treated as a column in a matrix transform. Example: the 5th pin is `RC(0,4)`.

`toggle-mode` minimizes pull-resistor current for switches that may stay in the active state for long periods.

- It applies to all switches handled by that driver instance.
- Each pole of the toggle switch needs its own GPIO.
- To mix toggle and non-toggle switches, create two direct-gpio instances and combine them with a composite driver.
- Switch state is sampled at power-on, so off-state changes are preserved.
- In toggle-mode, do not set pull resistors in Devicetree; the driver manages them.

Example SP3T toggle setup with the common pole tied to ground:

```dts
kscan_sp3t_toggle: kscan_sp3t_toggle {
    compatible = "zmk,kscan-gpio-direct";
    toggle-mode;

    input-gpios
    = <&pro_micro 4 GPIO_ACTIVE_LOW>
    , <&pro_micro 3 GPIO_ACTIVE_LOW>
    , <&pro_micro 2 GPIO_ACTIVE_LOW>
    ;
};
```

### Matrix Driver

Keys are arranged as a row/column matrix.

#### Matrix Kconfig

Definition file: `zmk/app/module/drivers/kscan/Kconfig`

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_KSCAN_MATRIX_POLLING | bool | Poll for key presses instead of using interrupts | n |
| CONFIG_ZMK_KSCAN_MATRIX_WAIT_BEFORE_INPUTS | int (ticks) | Wait before reading inputs after activating an output | 0 |
| CONFIG_ZMK_KSCAN_MATRIX_WAIT_BETWEEN_OUTPUTS | int (ticks) | Wait between outputs so the previous one settles | 0 |

#### Matrix Devicetree

Applies to: `compatible = "zmk,kscan-gpio-matrix"`

Definition file: `zmk/app/module/dts/bindings/kscan/zmk,kscan-gpio-matrix.yaml`

| Property | Type | Description | Default |
| --- | --- | --- | --- |
| row-gpios | GPIO array | Row GPIOs, top to bottom | |
| col-gpios | GPIO array | Column GPIOs, left to right | |
| debounce-press-ms | int | Press debounce in ms; use 0 for eager behavior | 5 |
| debounce-release-ms | int | Release debounce in ms | 5 |
| debounce-scan-period-ms | int | Scan interval while any key is pressed | 1 |
| diode-direction | string | Matrix diode direction | row2col |
| poll-period-ms | int | Scan interval when idle and polling is enabled | 10 |
| wakeup-source | bool | Allow this kscan to wake the keyboard | n |

`diode-direction` must be one of:

| Value | Meaning |
| --- | --- |
| row2col | Diodes point from rows to columns; cathodes on columns |
| col2row | Diodes point from columns to rows; cathodes on rows |

GPIO flags must match the diode direction. For `col2row`, columns are outputs and should use `GPIO_ACTIVE_HIGH`; rows are inputs and should use `GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN`.

```dts
kscan0: kscan {
    compatible = "zmk,kscan-gpio-matrix";
    wakeup-source;
    diode-direction = "col2row";
    col-gpios
        = <&pro_micro 4 GPIO_ACTIVE_HIGH>
        , <&pro_micro 5 GPIO_ACTIVE_HIGH>
        ;
    row-gpios
        = <&pro_micro 6 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
        , <&pro_micro 7 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>
        ;
};
```

### Charlieplex Driver

Each GPIO is used as both input and output.

- Without `interrupt-gpios`, n pins can address `n x (n - 1)` keys.
- With `interrupt-gpios`, n pins can address `(n - 1) x (n - 2)` keys, but with much better power behavior.

#### Charlieplex Kconfig

Definition file: `zmk/app/module/drivers/kscan/Kconfig`

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_KSCAN_CHARLIEPLEX_WAIT_BEFORE_INPUTS | int (ticks) | Wait before reading inputs after activating an output | 0 |
| CONFIG_ZMK_KSCAN_CHARLIEPLEX_WAIT_BETWEEN_OUTPUTS | int (ticks) | Wait between outputs so the previous one settles | 0 |

#### Charlieplex Devicetree

Applies to: `compatible = "zmk,kscan-gpio-charlieplex"`

Definition file: `zmk/app/module/dts/bindings/kscan/zmk,kscan-gpio-charlieplex.yaml`

| Property | Type | Description | Default |
| --- | --- | --- | --- |
| gpios | GPIO array | GPIOs used, in order | |
| interrupt-gpios | GPIO array | Single interrupt GPIO; if omitted, polling is continuous | |
| debounce-press-ms | int | Press debounce in ms; use 0 for eager behavior | 5 |
| debounce-release-ms | int | Release debounce in ms | 5 |
| debounce-scan-period-ms | int | Scan interval while any key is pressed | 1 |
| poll-period-ms | int | Scan interval when idle and `interrupt-gpios` is unset | 10 |
| wakeup-source | bool | Allow this kscan to wake the keyboard | n |

Use a matrix transform with charlieplex. In `RC(r,c)`, the row is the driven pin and the column is the receiving pin. Example: `RC(5,0)` drives from the 6th GPIO to the 1st GPIO. Exclude all `row == column` positions, because no pin can be input and output simultaneously.

- `gpios` should use `GPIO_ACTIVE_HIGH`.
- `interrupt-gpios` should use `GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN`.

### Composite Driver

Combines multiple keyboard scan drivers.

#### Composite Devicetree

Applies to: `compatible = "zmk,kscan-composite"`

Definition file: `zmk/app/dts/bindings/zmk,kscan-composite.yaml`

| Property | Type | Description | Default |
| --- | --- | --- | --- |
| rows | int | Number of rows in the composite matrix | |
| columns | int | Number of columns in the composite matrix | |
| wakeup-source | bool | Allow this kscan to wake the keyboard | n |

Each child node corresponds to one included driver:

| Child Property | Type | Description | Default |
| --- | --- | --- | --- |
| kscan | phandle | Included kscan driver | |
| row-offset | int | Shift included row 0 to a new row | 0 |
| col-offset | int | Shift included col 0 to a new column | 0 |

If an included kscan should wake the keyboard, set `wakeup-source` on both the child kscan itself and on the composite node.

#### Example Configuration

Example: a macropad with a 3x3 matrix plus two direct GPIO keys.

Matrix:

```text
      Col 0 Col 1 Col 2
Row 0 A0    A1    A2
Row 1 A3    A4    A5
Row 2 A6    A7    A8
```

Direct GPIO:

```text
      Col 0 Col 1
Row 0 B0    B1
```

One valid composite arrangement is a 4x3 matrix with the direct keys placed on row 3:

```text
      Col 0 Col 1 Col 2
Row 0 A0    A1    A2
Row 1 A3    A4    A5
Row 2 A6    A7    A8
Row 3 B0    B1    (none)
```

```dts
/ {
    chosen {
        zmk,kscan = &kscan0;
    };

    kscan0: kscan_composite {
        compatible = "zmk,kscan-composite";
        rows = <4>;
        columns = <3>;

        matrix {
            kscan = <&kscan1>;
        };

        direct {
            kscan = <&kscan2>;
            row-offset = <3>;
        };
    };

    kscan1: kscan_matrix {
        compatible = "zmk,kscan-gpio-matrix";
        // define 3x3 matrix here...
    };

    kscan2: kscan_direct {
        compatible = "zmk,kscan-gpio-direct";
        // define 2 direct GPIOs here...
    };
};
```

### Mock Driver

Simulates key events.

#### Mock Devicetree

Applies to: `compatible = "zmk,kscan-mock"`

Definition file: `zmk/app/dts/bindings/zmk,kscan-mock.yaml`

| Property | Type | Description | Default |
| --- | --- | --- | --- |
| event-period | int | Milliseconds between generated events | |
| events | array | Key events to simulate | |
| rows | int | Number of rows in the composite matrix | |
| columns | int | Number of columns in the composite matrix | |
| exit-after | bool | Exit after all events have run | false |

Define `events` with the macros in `app/module/include/dt-bindings/zmk/kscan_mock.h`.

### Kscan Sideband Behavior Driver

Lets you assign behaviors to specific keys independently of the keymap. These assignments do not affect, and are not affected by, the keymap.

#### Kscan Sideband Devicetree

Applies to: `compatible = "zmk,kscan-sideband-behaviors"`

Definition file: `zmk/app/dts/bindings/kscan/zmk,matrix-transform.yaml`

| Property | Type | Description |
| --- | --- | --- |
| kscan | phandle | Kscan containing the keys to intercept |
| auto-enable | bool | Enable sideband behavior unconditionally at startup |
| wakeup-source | bool | Allow this kscan instance to wake the keyboard |

If `auto-enable` is not set, the sideband instance waits for an external activation source, such as being assigned to the chosen `zmk,kscan`. The kscan may contain extra keys that are still used by the keymap if this node becomes the chosen `zmk,kscan` and a suitable matrix transform exists.

Each child node defines one intercepted key:

| Child Property | Type | Description | Default |
| --- | --- | --- | --- |
| row | int | Row index to intercept | 0 |
| column | int | Column index to intercept | |
| bindings | phandle-array | Behavior to trigger on that event | |

## Split Configuration

These settings control split-keyboard behavior.

See Configuration Overview for how to apply them.

### Split Kconfig

Defined in `zmk/app/src/split/Kconfig`.

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_SPLIT | bool | Enable split keyboard support | n |
| CONFIG_ZMK_SPLIT_ROLE_CENTRAL | bool | y for central, n for peripheral | n |
| CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS | bool | Pass HID indicator state to peripherals | n |

### Bluetooth Splits

Defined in `zmk/app/src/split/bluetooth/Kconfig`.

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_SPLIT_BLE | bool | Use BLE between split halves | y |
| CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS | int | Number of peripherals connected to the central | 1 |
| CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING | bool | Fetch peripheral battery levels on the central | n |
| CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY | bool | Report split battery levels from the central to hosts | n |
| CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_QUEUE_SIZE | int | Max queued peripheral battery events on the central | CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS |
| CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE | int | Max queued key-state events received by the central | 5 |
| CONFIG_ZMK_SPLIT_BLE_CENTRAL_SPLIT_RUN_STACK_SIZE | int | BLE split central write-thread stack size | 512 |
| CONFIG_ZMK_SPLIT_BLE_CENTRAL_SPLIT_RUN_QUEUE_SIZE | int | Max queued behavior-run events sent to peripherals | 5 |
| CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_STACK_SIZE | int | BLE split peripheral notify-thread stack size | 756 |
| CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_PRIORITY | int | BLE split peripheral notify-thread priority | 5 |
| CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_POSITION_QUEUE_SIZE | int | Max queued key-state events sent to the central | 10 |

### Wired Splits

UART support varies by hardware and driver, so ZMK supports multiple UART interaction modes. The default mode is chosen based on platform support, but you can override it.

- Polling mode: least efficient; MCU repeatedly polls UART for incoming data. Supported by all UART drivers.
- Interrupt mode: MCU can do other work until UART raises an interrupt. FIFO-backed implementations can be faster and more efficient. Examples: RP2040, nRF52.
- Async (DMA) mode: reception can happen without the MCU, and larger transfers can be copied directly to accessible memory. This is the most efficient option where available. Examples: SAM0 such as SAMD21, STM32 such as stm32f072. Current Zephyr 3.5 nRF52 UART support has bugs that prevent using it there.

Defined in `zmk/app/src/split/wired/Kconfig`.

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_SPLIT_WIRED | bool | Use wired communication between halves | y if devicetree is configured appropriately |
| CONFIG_ZMK_SPLIT_WIRED_UART_MODE_ASYNC | bool | Async DMA UART mode | y if supported, except nRF52 with known bugs |
| CONFIG_ZMK_SPLIT_WIRED_UART_MODE_INTERRUPT | bool | Interrupt UART mode | y if hardware supports it |
| CONFIG_ZMK_SPLIT_WIRED_UART_MODE_POLLING | bool | Polling UART mode | y if neither other mode is supported |

#### Async (DMA) Mode

These apply only in async mode:

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_SPLIT_WIRED_ASYNC_RX_TIMEOUT | int | RX timeout in microseconds before reporting received data | 20 |

#### Polling Mode

These apply only in polling mode:

| Config | Type | Description | Default |
| --- | --- | --- | --- |
| CONFIG_ZMK_SPLIT_WIRED_POLLING_RX_PERIOD | int | Ticks between split-data polling calls | 10 |

### Split Devicetree

#### Wired Split

Wired splits need a correctly configured UART. If you are writing a shield, you may be able to reuse a standard UART already exposed by the board, such as `&pro_micro_serial`. For custom boards or custom pins, configure the UART through pin control.

Once the UART device exists, assign it to a node with `compatible = "zmk,wired-split"`:

```dts
/ {
    wired_split {
        compatible = "zmk,wired-split";
        device = <&pro_micro_serial>;
    };
};
```
