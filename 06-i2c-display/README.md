# Example 06: I2C SSD1306 Display

This example drives a four-pin SSD1306-compatible 128x64 OLED through the
STM32F103 hardware I2C1 peripheral.

## Hardware

The display module must expose:

```text
GND  VCC  SCL  SDA
```

Connect it to the Blue Pill as follows:

```text
Blue Pill      OLED
---------      ----
GND        ->  GND
3.3V       ->  VCC
PB6        ->  SCL
PB7        ->  SDA
```

Use 3.3 V logic. Most four-pin modules contain I2C pull-up resistors. If
your module does not, add pull-ups from SCL and SDA to 3.3 V.

## Default bus configuration

```text
Peripheral:       I2C1
SCL:              PB6
SDA:              PB7
Clock:            400 kHz
7-bit address:    0x3C
Transfer mode:    polling with timeout
Display:          SSD1306-compatible 128x64 monochrome
Framebuffer:      1024 bytes
```

Some modules use address `0x3D`. Change
`BOARD_DISPLAY_I2C_ADDRESS_7BIT` in `config/board_config.h` when needed.

## Architecture

```text
Application
    |
    v
Display Service
    |
    v
SSD1306 ECUAL
    |
    v
Board Display Bus
    |
    v
I2C1 / GPIO
```

The SSD1306 driver owns controller commands, drawing primitives, the
5x7 font, and the framebuffer. The BSP owns only the STM32-specific I2C
transport.

I2C uses SSD1306 control bytes:

```text
0x00 -> following bytes are commands
0x40 -> following bytes are display data
```

## Demonstration

The screen displays:

```text
STM32F103
I2C SSD1306

UPTIME <seconds>
SECONDS

[progress bar]
```

The progress bar changes every 100 ms. A full framebuffer refresh sends
1024 display bytes over I2C.

## Idle policy

`system_idle()` uses `__NOP()` rather than `__WFI()`. This keeps SWD
attachment reliable with the user's ST-Link probe, which has no physical
NRST connection.

## Build and flash

```bash
make check-layers
make clean
make
make flash
```

## Configuration

`config/board_config.h`:

```c
#define BOARD_DISPLAY_I2C_ADDRESS_7BIT  (0x3CU)
#define BOARD_DISPLAY_I2C_CLOCK_HZ      (400000UL)
#define BOARD_DISPLAY_I2C_TIMEOUT_MS    (20UL)
#define BOARD_DISPLAY_POWER_ON_DELAY_MS (100UL)
```

`config/application_config.h` controls the animation period and step.

## Troubleshooting

A completely blank display usually means one of these:

- SCL and SDA are swapped.
- The display address is `0x3D` instead of `0x3C`.
- The module is 128x32 rather than 128x64.
- The module lacks pull-up resistors.
- The module was powered from an unsuitable voltage.

The firmware returns failure and enters `system_panic()` if the OLED does
not acknowledge its I2C address during initialization.
