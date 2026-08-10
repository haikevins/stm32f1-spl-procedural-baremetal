# 06 - I2C SSD1306 Display

## Purpose

Drive a four-pin 128x64 SSD1306 OLED over I2C1 using a BSP bus layer, ECUAL display driver, framebuffer, text rendering, and a progress-bar demo.

This project is independently buildable and uses the same layered architecture
as the rest of the repository.

## Learning Goals

By the end of this example, you should be able to:

- trace initialization from `main()` through System, BSP, Services, and
  Application;
- identify which layer owns each physical peripheral;
- explain the runtime data/control flow;
- distinguish ISR work from thread-mode work where interrupts are used;
- modify compile-time configuration without violating dependency direction;
- debug the example from the hardware layer upward.

## Hardware and Wiring


Connect the four-pin OLED:

```text
Blue Pill      OLED
-------------------
GND        --> GND
3.3V       --> VCC
PB6        --> SCL
PB7        --> SDA
```

The example uses I2C1. The default 7-bit display address is `0x3C`.

Most common four-pin modules include pull-up resistors. If yours does not, SCL
and SDA require pull-ups to 3.3 V.


## Compile-Time Configuration


| Setting | Value |
|---|---|
| I2C peripheral | I2C1 |
| SCL | PB6 |
| SDA | PB7 |
| Address | 0x3C |
| I2C clock | 400 kHz |
| I2C transaction timeout | 20 ms |
| OLED power-on delay | 100 ms |
| Timebase | 1 ms |
| Display | 128x64 |
| Framebuffer | 1024 bytes |
| Demo update | 100 ms |
| Progress step | 2% |

Change the address to `0x3D` in `board_config.h` only if the actual module uses
that address.


## Initialization Sequence


Board initialization starts the timebase first because I2C polling and OLED
power-on delay depend on it.

```text
board_init()
    |
    +--> board_timebase_init()
    +--> board_display_bus_init()
            |
            +--> GPIOB clock
            +--> PB6/PB7 AF open-drain
            +--> I2C1 clock
            +--> I2C initialization
            +--> wait for bus idle

display_service_init()
    |
    +--> ssd1306_init()
            |
            +--> wait power-on delay
            +--> send SSD1306 init commands
            +--> clear framebuffer
            +--> transfer framebuffer
```


## Runtime Behavior


The Application renders:

```text
STM32F103
I2C SSD1306

UPTIME <seconds>
SECONDS

[progress bar]
```

Every 100 ms it updates the progress percentage, rebuilds the framebuffer, and
calls `display_service_present()`.

The SSD1306 ECUAL keeps a 1024-byte framebuffer arranged as 8 pages × 128
columns. Pixel, text, and progress-bar functions only manipulate RAM. The
`update` operation sends the display address window followed by framebuffer
data.


## SPL / Low-Level Behavior


The BSP I2C transaction is polling and bounded:

1. wait until BUSY clears;
2. generate START;
3. wait for master mode;
4. send 7-bit address in transmitter mode;
5. send SSD1306 control byte (`0x00` command or `0x40` data);
6. send payload bytes;
7. wait for final byte transmitted;
8. generate STOP.

BERR, ARLO, AF, OVR, and TIMEOUT flags are checked and cleared on abort.

There is no I2C interrupt handler in this example.


## Architectural Notes


This example introduces a real ECUAL driver. SSD1306 code does not need to know
PB6/PB7 or I2C1. It uses the board display bus API. The Service gives the
Application a display capability rather than exposing SSD1306 commands.


## Interrupt and Concurrency Policy

The project follows the repository-wide rule that an interrupt handler belongs
to the lowest module that owns the peripheral. The ISR, when present, may clear
flags, transfer low-level data, and record bounded state. Higher-level policy is
processed later in normal thread mode.

`system_idle()` in this concrete example executes `__NOP()` rather than
`__WFI()`.

## Test Procedure and Expected Result


Power the OLED from 3.3 V, flash the firmware, and reset.

Expected display:

- `STM32F103`;
- `I2C SSD1306`;
- uptime increasing in seconds;
- a progress bar repeatedly filling and emptying.

If the screen is blank:

1. verify VCC/GND;
2. verify PB6/PB7 are not swapped;
3. confirm address `0x3C` vs `0x3D`;
4. verify pull-ups;
5. break in `board_display_bus_write()`;
6. identify whether failure occurs at BUSY, START, address ACK, or data phase.


## GDB Debugging


Useful breakpoints:

```gdb
break board_display_bus_init
break board_display_bus_write
break ssd1306_init
break ssd1306_update
break application_process
continue
```

Useful Application state in context:

```gdb
p s_progress_percent
p s_progress_increasing
p s_display_operational
```

If `s_display_operational` becomes false after startup, a later I2C update
failed and the Application intentionally stops attempting new frames.



## Build, Flash, and Debug

Run the commands from the example directory.

```bash
make check-layers
make clean
make
```

The build produces:

```text
build/firmware.elf
build/firmware.hex
build/firmware.bin
build/firmware.lst
build/firmware.map
```

Flash with OpenOCD:

```bash
make flash
```

Erase the MCU flash if needed:

```bash
make erase
```

Start an OpenOCD debug server:

```bash
make debug-server
```

Then, in another terminal:

```bash
make debug
```

The Makefile prefers `arm-none-eabi-gdb` and falls back to `gdb-multiarch`.

The OpenOCD configuration uses SWD and:

```tcl
reset_config none
adapter speed 1000
```

This matches a common ST-Link connection where only `SWDIO`, `SWCLK`, `GND`,
and `3.3V` are connected and NRST is not available.


## Troubleshooting Method

Use a bottom-up approach:

1. verify power and wiring;
2. verify BSP pin/peripheral mapping;
3. verify the peripheral clock is enabled;
4. verify initialization succeeds;
5. verify the low-level peripheral flag/interrupt/data path;
6. verify Service state;
7. verify Application policy.

Do not immediately modify Application code when the underlying peripheral is
not yet proven to work.

## Porting Notes


To move I2C pins or the peripheral, update the BSP mapping and bus
initialization. To use another OLED controller, keep the board I2C bus and
replace the ECUAL driver. To change display dimensions, framebuffer layout and
rendering bounds must be reviewed together.


See [`docs/porting_guide.md`](docs/porting_guide.md) for a structured checklist.

## Further Exercises

Good next experiments include:

- expose additional diagnostic counters through GDB;
- add a second logical Service without letting Application include BSP headers;
- deliberately inject a failure and trace how it propagates;
- write a host-side test for portable Common or Service logic;
- change one board resource and verify that Application does not need hardware
  includes.

## Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/adding_a_module.md`](docs/adding_a_module.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
