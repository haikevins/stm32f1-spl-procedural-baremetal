# 06-i2c-display — I2C1 + SSD1306 OLED 128x64

## 1. Learning Objectives

This example introduces an external device with a dedicated ECUAL driver.

You will learn:

- I2C1 on PB6/PB7;
- alternate-function open-drain GPIO;
- bounded SPL I2C polling;
- SSD1306 command vs data control bytes;
- display power-on sequencing;
- framebuffer ownership;
- simple 5x7 text rendering;
- progress-bar rendering;
- separation between Application, Service, ECUAL, and board bus.

## 2. Hardware and Wiring

Connect a four-pin SSD1306-compatible I2C module:

```text
Blue Pill      OLED
-------------------
GND        --> GND
3.3V       --> VCC
PB6        --> SCL
PB7        --> SDA
```

Default 7-bit address:

```text
0x3C
```

Some modules use `0x3D`.

I2C requires pull-ups to 3.3 V. Many modules already include them.

## 3. Expected Display

The demo renders:

```text
STM32F103
I2C SSD1306

UPTIME <seconds>
SECONDS

[progress bar]
```

The progress bar repeatedly fills and empties.

The frame is refreshed every 100 ms.

## 4. Compile-Time Configuration

```c
#define BOARD_TIMEBASE_HZ                (1000UL)
#define BOARD_DISPLAY_I2C_ADDRESS_7BIT   (0x3CU)
#define BOARD_DISPLAY_I2C_CLOCK_HZ       (400000UL)
#define BOARD_DISPLAY_I2C_TIMEOUT_MS     (20UL)
#define BOARD_DISPLAY_POWER_ON_DELAY_MS  (100UL)

#define DISPLAY_DEMO_UPDATE_PERIOD_MS    (100UL)
#define DISPLAY_DEMO_PROGRESS_STEP       (2U)
```

## 5. Pin Configuration

PB6 and PB7 are configured as alternate-function open-drain outputs:

```text
PB6 -> I2C1_SCL
PB7 -> I2C1_SDA
```

Open-drain is required because I2C devices only actively pull the bus low.

The pull-up resistors define the HIGH level.

## 6. I2C Clock — Normal 72 MHz System Clock

I2C1 is on APB1.

The SPL `I2C_Init()` routine receives:

```text
I2C_ClockSpeed = 400000
```

and configures the peripheral timing from the APB1 clock.

For a typical 72 MHz system configuration:

```text
PCLK1 = 36 MHz
requested I2C = 400 kHz
```

Do not hard-code CCR values in Application.

## 7. Clock Changes

If the system/APB1 clock changes, the I2C timing must still be derived from the
actual peripheral clock.

SPL performs the register calculation, but the BSP remains responsible for
initializing I2C after the clock tree is valid.

Always verify SCL frequency with a logic analyzer after clock-tree changes.

## 8. I2C Initialization Sequence

`board_display_bus_init()`:

1. enables GPIOB and I2C1 clocks;
2. configures PB6/PB7 as AF open-drain;
3. deinitializes I2C1;
4. configures clock speed and standard I2C parameters;
5. enables I2C1;
6. clears stale error flags;
7. waits for BUSY to clear.

The board timebase is initialized before the display bus because polling
timeouts depend on milliseconds.

## 9. Write Transaction

The board bus performs a bounded write transaction.

Conceptually:

```text
wait BUSY clear
    |
START
    |
wait master mode
    |
send 7-bit address + write
    |
wait transmitter mode
    |
send SSD1306 control byte
    |
send payload bytes
    |
wait final byte transmitted
    |
STOP
```

Timeouts prevent an unbounded I2C wait.

## 10. SSD1306 Control Bytes

The board bus prepends:

```text
0x00 -> command stream
0x40 -> display RAM data stream
```

The ECUAL driver chooses whether a transfer is commands or data.

## 11. ECUAL Transport Design

Dependency:

```text
SSD1306 ECUAL
    |
Board Display Bus
    |
I2C1 SPL
```

The SSD1306 driver does not know PB6/PB7.

It calls:

```c
board_display_bus_write(data_mode, bytes, length);
```

This keeps the display protocol separate from MCU wiring.

## 12. Power-On Delay

Four-pin modules often do not expose the SSD1306 RESET pin.

The driver therefore waits:

```text
100 ms
```

before sending initialization commands.

The delay uses the board timebase rather than an arbitrary empty loop.

## 13. SSD1306 Initialization

The driver sends a command sequence that configures:

- display off during setup;
- clock/oscillator;
- 64-row multiplex;
- display offset/start line;
- charge pump;
- horizontal addressing;
- segment remap;
- COM scan direction;
- COM pin configuration;
- contrast;
- pre-charge;
- VCOMH;
- normal display mode;
- scroll off;
- display on.

After initialization, the framebuffer is cleared and presented.

## 14. Framebuffer Layout

Display size:

```text
128 x 64 pixels
```

Pages:

```text
64 / 8 = 8 pages
```

Framebuffer:

```text
128 * 8 = 1024 bytes
```

Pixel index:

```text
index = x + (y / 8) * 128
mask  = 1 << (y % 8)
```

The framebuffer is statically allocated.

## 15. Text Renderer

The ECUAL contains a compact 5x7 font.

Glyph properties:

```text
width: 5 pixels
height: 7 pixels
advance: 6 pixels
```

Unknown characters fall back to the space glyph.

The renderer clips when the next character would exceed the display bounds.

## 16. Progress Bar

The progress-bar function:

1. draws a rectangular border;
2. clamps percent to 100;
3. calculates interior fill width;
4. writes filled/unfilled pixels.

Application only passes logical geometry and percentage.

## 17. Display Update

`ssd1306_update()` sends:

```text
column address: 0..127
page address:   0..7
```

then transmits the entire 1024-byte framebuffer.

This is simple and deterministic, though not bandwidth-optimal.

## 18. Error Handling

The board bus checks/clears I2C errors such as:

```text
BERR
ARLO
AF
OVR
TIMEOUT
```

Initialization or update returns `false` on failure.

If a runtime `display_service_present()` fails, the Application marks the
display non-operational and stops attempting further updates.

## 19. Interrupt Policy

I2C interrupts are not used.

The bus is synchronous/polling with explicit timeouts.

SysTick provides the millisecond timebase.

## 20. Architecture

```text
Application
    |
Display Service
    |
SSD1306 ECUAL
    |
Board Display Bus
    |
I2C/GPIO/RCC SPL
```

Time Service/Board Timebase provide scheduling and timeout support.

## Build, Flash, and Debug

```bash
make check-layers
make clean
make
make flash
```

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

## 21. Test Procedure

1. Verify 3.3 V/GND.
2. Verify PB6=SCL, PB7=SDA.
3. Confirm pull-ups exist.
4. Flash firmware.
5. Verify text appears.
6. Verify uptime increments.
7. Verify progress bar moves.
8. If available, measure SCL near 400 kHz.
9. Reset several times and verify reliable initialization.

## 22. Troubleshooting

### OLED Is Completely Black

Check:

- power;
- common ground;
- SCL/SDA wiring;
- address 0x3C vs 0x3D;
- pull-ups;
- initialization return value.

### I2C BUSY Never Clears

Possible causes:

- SDA stuck low;
- SCL stuck low;
- wrong wiring;
- device held/reset improperly;
- previous interrupted transaction.

Inspect both lines with a meter or logic analyzer.

### Text Is Shifted or Inverted

Check SSD1306 variant/orientation assumptions:

- segment remap;
- COM scan direction;
- display geometry;
- controller compatibility.

### Update Error After Running

Inspect:

- I2C error flags;
- supply stability;
- pull-up strength;
- bus capacitance;
- update return value.

## 23. Extension Exercises

1. Add lowercase glyphs.
2. Add partial-page updates.
3. Add bitmap drawing.
4. Add a display reset GPIO.
5. Add an I2C address scan tool.
6. Add an asynchronous display-update Service.
7. Support another OLED controller.

## 24. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
