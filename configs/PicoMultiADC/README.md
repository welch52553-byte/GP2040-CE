# PicoMultiADC Board Configuration

Board configuration for Raspberry Pi Pico with multi-channel ADC (analog-to-digital converter) support for analog signal input.

## Features

- **4 ADC channels** (GPIO 26-29 / ADC0-ADC3) with flexible mapping
- **On-chip temperature sensor** readout via ADC4
- **Oversampling** (configurable, default 4x) for noise reduction
- **EMA smoothing** for stable analog readings
- **Radial deadzone** with configurable inner/outer ranges
- **Auto-calibration** on startup

## Pin Mapping

| GPIO | Function | ADC Channel |
|------|----------|-------------|
| 26   | Left Stick X  | ADC0 |
| 27   | Left Stick Y  | ADC1 |
| 28   | Right Stick X | ADC2 |
| 29   | Right Stick Y | ADC3 |

### Channel Mapping Options

Each ADC channel can be mapped to:
- `LEFT_STICK_X` / `LEFT_STICK_Y` - Left analog stick axes
- `RIGHT_STICK_X` / `RIGHT_STICK_Y` - Right analog stick axes
- `LEFT_TRIGGER` / `RIGHT_TRIGGER` - Analog triggers (L2/R2)
- `RAW_OUTPUT` - Raw ADC value (accessible via API)
- `NONE` - Channel disabled

## Build

```bash
# Using build script
./scripts/build.sh PicoMultiADC

# Or using CMake directly
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DPICO_SDK_PATH=/opt/pico-sdk \
    -DGP2040_BOARDCONFIG=PicoMultiADC \
    -DSKIP_WEBBUILD=TRUE ..
cmake --build . --config Release
```

## Wiring

Connect analog sources (potentiometers, joysticks, sensors) to GPIO 26-29:

```
Analog Source     Pico
┌─────────┐      ┌──────────┐
│ VCC ────┼──────┤ 3V3 (36) │
│ GND ────┼──────┤ GND (38) │
│ OUT ────┼──────┤ GP26 (31)│  <- ADC0
│ OUT ────┼──────┤ GP27 (32)│  <- ADC1
│ OUT ────┼──────┤ GP28 (34)│  <- ADC2
│ OUT ────┼──────┤ GP29 (35)│  <- ADC3 (if not using VSYS sense)
└─────────┘      └──────────┘
```

> **Note:** GPIO 29 (ADC3) is connected to VSYS/3 on the Pico board via a voltage divider.
> If using an unmodified Pico, you may only have 3 external ADC channels (ADC0-ADC2).
> On Pico W, GPIO 29 is used for WiFi, so only ADC0-ADC2 are available.
