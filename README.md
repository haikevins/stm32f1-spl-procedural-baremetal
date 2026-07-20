# Example 01: Non-Blocking LED Blink

A layered bare-metal example for the **STM32F103C8T6 Blue Pill** using **CMSIS** and the **STM32F10x Standard Peripheral Library (SPL)**.

The onboard active-low LED connected to **PC13** toggles every **500 ms**. SysTick provides a millisecond timebase, the Application layer uses a non-blocking periodic check, and the super-loop enters `WFI` while no processing is required.

## Learning Objectives

This example demonstrates:

- GPIO output configuration with STM32F10x SPL
- Active-low LED control on the Blue Pill
- SysTick-based millisecond timekeeping
- Non-blocking periodic execution without a busy-wait delay
- Separation between Application, Services, BSP, SPL, and CMSIS
- System-level initialization through a composition root
- Build, flash, and debug workflows using GNU Make and OpenOCD

## Target

| Item | Value |
|---|---|
| MCU | STM32F103C8T6 |
| Board | STM32F103C8T6 Blue Pill |
| LED | Onboard status LED |
| GPIO | PC13 |
| Electrical behavior | Active-low |
| Toggle period | 500 ms |
| Peripheral library | STM32F10x SPL |
| Core support | CMSIS |
| Build system | GNU Make |
| Programmer/debugger | ST-Link through SWD |

## Expected Behavior

```text
PC13 low  -> LED on
PC13 high -> LED off

Toggle interval: 500 ms
Complete on/off cycle: 1 second
```

## Software Flow

### Initialization

```text
main()
    |
    v
system_init()
    |
    +--> board_init()
    |       |
    |       +--> board LED initialization
    |       |
    |       +--> SysTick timebase initialization
    |
    +--> time_service_init()
    |
    +--> indication_service_init()
    |
    +--> application_init()
```

### Runtime

```text
Super-loop
    |
    v
application_process()
    |
    +--> time_service_periodic_due(..., 500 ms)
            |
            +--> not due: return
            |
            +--> due
                    |
                    v
            indication_service_toggle(INDICATION_STATUS)
                    |
                    v
                Board LED
                    |
                    v
              STM32 SPL GPIO
                    |
                    v
                  PC13
    |
    v
system_idle() -> WFI
```

## Dependency Paths

```text
Application
    -> Time Service
        -> Board Timebase
            -> CMSIS SysTick

Application
    -> Indication Service
        -> Board LED
            -> STM32 SPL GPIO
```

Application code does not include BSP, SPL, CMSIS, or STM32 device headers.

## Layer Responsibilities

| Layer or file | Responsibility |
|---|---|
| `app/src/application.c` | Defines blink policy and requests a status-indication toggle every configured period |
| `config/application_config.h` | Stores the application blink period |
| `services/src/time_service.c` | Provides hardware-independent time and periodic scheduling APIs |
| `services/src/indication_service.c` | Exposes a logical indication interface to the Application layer |
| `bsp/bluepill/src/board.c` | Initializes board-owned resources |
| `bsp/bluepill/src/board_led.c` | Configures and controls the physical PC13 LED through SPL |
| `bsp/bluepill/src/board_timebase.c` | Configures SysTick and maintains the millisecond counter |
| `bsp/bluepill/src/board_pins.h` | Maps the logical status LED to the physical GPIO |
| `system/system_init.c` | Defines initialization order and connects the layers |
| `system/main.c` | Runs the super-loop and idle policy |
| `config/modules.mk` | Selects the required SPL implementation sources |

## Project Layout

```text
01-blink-led/
├── app/                         Blink behavior
├── services/                    Time and indication abstractions
├── bsp/bluepill/                LED and timebase implementation
├── ecual/                       External-device layer placeholder
├── common/                      Portable shared utilities
├── config/                      Application, board, and SPL configuration
├── system/                      Initialization and super-loop
├── platform/                    Platform-level helpers
├── runtime/                     Minimal C runtime support
├── startup/                     Reset handler and vector table
├── linker/                      STM32F103C8T6 linker script
├── third_party/
│   ├── CMSIS/
│   └── STM32F10x_StdPeriph_Driver/
├── tests/host/
├── tools/
├── docs/
├── Makefile
└── README.md
```

## Prerequisites

- GNU Arm Embedded Toolchain
- GNU Make
- Python 3
- OpenOCD
- `arm-none-eabi-gdb` or `gdb-multiarch`
- ST-Link or another OpenOCD-compatible SWD probe
- STM32F103C8T6 Blue Pill board

## Build

```bash
make
```

The default build produces:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

Use a custom output name:

```bash
make PROJECT=01-blink-led
```

Check architecture dependencies explicitly:

```bash
make check-layers
```

Display firmware size:

```bash
make size
```

Clean generated files:

```bash
make clean
```

## Flash

Connect the Blue Pill to an ST-Link through SWD, then run:

```bash
make flash
```

Erase the MCU when required:

```bash
make erase
```

The OpenOCD configuration used by the project is:

```text
tools/openocd/bluepill_stlink.cfg
```

## Debug

Start OpenOCD in the first terminal:

```bash
make debug-server
```

Start GDB in a second terminal:

```bash
make debug
```

## Configuration

### Blink Period

Edit:

```text
config/application_config.h
```

Default value:

```c
#define APPLICATION_BLINK_PERIOD_MS (500UL)
```

For example, to toggle every 100 ms:

```c
#define APPLICATION_BLINK_PERIOD_MS (100UL)
```

### Timebase Frequency

The board timebase configuration is located in:

```text
config/board_config.h
```

### Board Pin Mapping

The status LED mapping and active level are defined in:

```text
bsp/bluepill/src/board_pins.h
```

### SPL Module Selection

The required STM32F10x SPL source files are selected in:

```text
config/modules.mk
```

## Design Notes

- The Application layer contains no direct GPIO code.
- The blink timing does not use a blocking delay loop.
- SysTick only updates a low-level time counter.
- Application behavior executes in normal thread mode, not inside the interrupt handler.
- The Indication Service hides the physical LED implementation.
- The BSP owns PC13 polarity and GPIO configuration.
- `system/` performs initialization and composition but contains no blink policy.
- `make check-layers` should pass before the firmware is committed.

## Troubleshooting

### `arm-none-eabi-gcc` not found

Install the GNU Arm Embedded Toolchain and verify:

```bash
arm-none-eabi-gcc --version
```

### OpenOCD cannot connect to the target

Check:

- ST-Link USB connection
- SWDIO, SWCLK, GND, and target voltage wiring
- Correct OpenOCD configuration
- Board power
- Whether another OpenOCD or GDB process is already using the probe

Then retry:

```bash
make flash
```

### Firmware flashes but the LED does not blink

Check:

- The board actually uses PC13 for the onboard LED
- The LED is active-low
- `APPLICATION_BLINK_PERIOD_MS` is non-zero
- SysTick initialization succeeds
- The firmware was built from the expected source tree
- The MCU was reset after programming

## License

This example is distributed under the MIT License. See [`LICENSE`](LICENSE).
