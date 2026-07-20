# STM32F1 SPL Procedural Bare-Metal Template 

A reusable, buildable project skeleton for the **STM32F103C8T6 Blue Pill**, written in C11 and ARM assembly using **CMSIS**, the **STM32F10x Standard Peripheral Library (SPL)**, and **GNU Make**.

This branch is intended to be cloned as the starting point for new SPL-based bare-metal projects. It provides startup code, a linker script, the vector table, a small runtime, build and debug tooling, architecture boundaries, and empty application hooks while deliberately leaving peripheral examples and product behavior to the user.

## Design Goals

- Procedural bare-metal development using CMSIS and STM32F10x SPL
- No STM32 HAL, LL, libopencm3, Arduino Core, or RTOS
- Clear separation between product logic and hardware-specific code
- Strict downward dependencies between architecture layers
- Non-blocking super-loop application design
- Small and understandable startup and runtime infrastructure
- Reusable foundation for GPIO, UART, SPI, I2C, CAN, ADC, DMA, timers, and external-device projects

## Target

| Item | Value |
|---|---|
| MCU | STM32F103C8T6 |
| Board | STM32F103C8T6 Blue Pill |
| CPU | Arm Cortex-M3 |
| Flash | 64 KiB |
| SRAM | 20 KiB |
| Language | C11 and GNU assembler |
| Peripheral library | STM32F10x Standard Peripheral Library |
| Core/device support | CMSIS |
| Build system | GNU Make |
| Debug interface | SWD through ST-Link or another OpenOCD-compatible probe |

## What This Template Contains

The default firmware performs only the minimum runtime sequence:

```text
Reset_Handler
    |
    v
Initialize .data and .bss
    |
    v
SystemInit()
    |
    v
main()
    |
    v
system_init()
    |
    +--> board_init()          // empty board hook
    |
    +--> application_init()    // empty application hook
    |
    v
Super-loop
    |
    +--> application_process() // empty application hook
    |
    +--> system_idle()         // WFI
```

The template does not include a functional GPIO, SysTick, UART, communication protocol, external-device driver, or product application. Complete projects are maintained on the repository's `examples` branch.

## Layered Architecture

Runtime dependencies must point downward:

```text
Application
    |
    v
Services
    |
    v
BSP / ECU Abstraction
    |
    v
STM32F10x Standard Peripheral Library
    |
    v
CMSIS
    |
    v
STM32F103 Hardware
```

The `system/` directory is the **composition root**. It may initialize and connect modules from multiple layers, but it must not contain product behavior.

Startup files, linker scripts, project configuration, vendor code, and development tools are infrastructure rather than runtime application layers.

### Layer Responsibilities

| Layer | Responsibility |
|---|---|
| Application | Product policy, state machines, and non-blocking application behavior |
| Services | Hardware-independent capabilities such as time, indications, communication, diagnostics, scheduling, and events |
| BSP | Mapping logical board resources to physical MCU pins and peripherals |
| ECU Abstraction | Drivers for external displays, sensors, memories, transceivers, and other off-chip devices |
| Common | Portable data types and utilities such as CRC, fixed-size queues, ring buffers, and bit helpers |
| System | Initialization order, module composition, super-loop control, idle behavior, and fatal-error policy |
| SPL | Vendor peripheral drivers used as the low-level peripheral layer |
| CMSIS | Cortex-M3 core support and STM32F103 device definitions |

### Dependency Rules

| Layer | May depend on | Must not depend on |
|---|---|---|
| Application | Services and portable Common APIs | BSP, ECUAL, SPL, CMSIS, or STM32 device headers |
| Services | BSP, ECUAL, and portable Common APIs | Application or raw STM32/SPL headers |
| BSP | SPL, CMSIS, and portable Common APIs | Services or Application |
| ECUAL | Preferably BSP bus APIs and portable Common APIs | Services or Application |
| Common | Standard language headers | Hardware-specific modules |
| System | Public APIs from all runtime layers | Product behavior |
| SPL / CMSIS | Vendor and architecture definitions | Upper project layers |

Run the dependency checker with:

```bash
make check-layers
```

The checker rejects forbidden project-header dependencies before the firmware is compiled.

## Repository Layout

```text
.
├── app/                         Product behavior and state machines
├── services/                    Hardware-independent services
├── ecual/                       External-device drivers
├── bsp/bluepill/                Blue Pill board abstraction
├── common/                      Portable utilities and shared types
├── config/                      Project and module configuration
├── system/                      Composition root and main super-loop
├── platform/                    Platform-level helpers
├── runtime/                     Minimal C runtime support
├── startup/                     Reset handler and vector table
├── linker/                      STM32F103C8T6 linker script
├── third_party/
│   ├── CMSIS/                   Cortex-M3 and STM32 device support
│   └── STM32F10x_StdPeriph_Driver/
├── tests/host/                  Host-side tests
├── tools/
│   ├── gdb/                     GDB command files
│   ├── openocd/                 OpenOCD configuration
│   └── scripts/                 Architecture and utility scripts
├── docs/                        Architecture and development guides
├── Makefile
└── README.md
```

## Prerequisites

Install the following tools:

- GNU Arm Embedded Toolchain:
  - `arm-none-eabi-gcc`
  - `arm-none-eabi-objcopy`
  - `arm-none-eabi-objdump`
  - `arm-none-eabi-size`
- GNU Make
- Python 3
- OpenOCD
- `arm-none-eabi-gdb` or `gdb-multiarch`
- ST-Link or another OpenOCD-compatible SWD probe

Verify the main tools:

```bash
arm-none-eabi-gcc --version
make --version
python3 --version
openocd --version
```

## Clone the Template Branch

```bash
git clone \
    --branch template \
    --single-branch \
    https://github.com/haikevins/stm32f1-spl-procedural-baremetal.git \
    my-stm32-project

cd my-stm32-project
```

To start a completely independent repository:

```bash
rm -rf .git
git init
git add .
git commit -m "chore: initialize STM32F103 SPL bare-metal project"
```

## Build

```bash
make
```

Generated files:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

Use a custom firmware name without editing the Makefile:

```bash
make PROJECT=my_firmware
```

Useful targets:

```bash
make check-layers   # Verify architecture dependencies
make size           # Display firmware section sizes
make tree           # Display the project structure
make clean          # Remove generated files
```

## Flash

Connect the Blue Pill to an ST-Link through SWD, then run:

```bash
make flash
```

Erase the device:

```bash
make erase
```

The default OpenOCD configuration is located at:

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

The Makefile uses `arm-none-eabi-gdb` when available and falls back to `gdb-multiarch`.

## Starting a New Project

A recommended implementation sequence is:

1. Define project-wide settings in `config/`.
2. Select the required SPL source files in `config/modules.mk`.
3. Add board resources in `bsp/bluepill/`.
4. Add external-device drivers in `ecual/`.
5. Add hardware-independent APIs in `services/`.
6. Implement product behavior in `app/`.
7. Connect module initialization in `system/system_init.c`.
8. Keep interrupt handlers in the lowest layer that owns the hardware resource.
9. Keep super-loop processing non-blocking.
10. Run `make check-layers` and `make` before committing.

### Module Placement

| Responsibility | Recommended location |
|---|---|
| Product state machine | `app/` |
| Time, events, indications, protocol, diagnostics | `services/` |
| Onboard LED, button, console, board timebase | `bsp/bluepill/` |
| External display, sensor, EEPROM, transceiver | `ecual/` |
| CRC, ring buffer, fixed-size queue | `common/` |
| Initialization wiring | `system/system_init.c` |
| Required SPL implementation sources | `config/modules.mk` |

A typical module should expose its public interface from an `include/` directory and keep implementation details in a `src/` directory.

## Interrupt Policy

An interrupt handler may:

- Read and acknowledge peripheral flags
- Transfer data into a statically allocated low-level buffer
- Update a low-level counter or status flag
- Wake normal thread-mode processing through a flag or queue

An interrupt handler must not:

- Include Application headers
- Run application state machines
- Parse high-level protocols
- Block or perform lengthy processing
- Call upward into Services or Application

Upper layers should consume interrupt-produced data through normal APIs during thread-mode execution.

## Documentation

- [`docs/architecture.md`](docs/architecture.md) — layer responsibilities and dependency direction
- [`docs/adding_a_module.md`](docs/adding_a_module.md) — recommended procedure for adding modules

## Branches

| Branch | Purpose |
|---|---|
| `template` | Reusable project skeleton without peripheral examples |
| `examples` | Complete, independently buildable demonstration projects |

## License

This project is distributed under the MIT License. See [`LICENSE`](LICENSE).
