# STM32F1 SPL Procedural Bare-Metal Examples

A collection of self-contained examples for the **STM32F103C8T6 Blue Pill**, implemented in C11 and ARM assembly using **CMSIS**, the **STM32F10x Standard Peripheral Library (SPL)**, and **GNU Make**.

These examples are derived from the project skeleton maintained on the repository's `template` branch. Each directory is an independent firmware project with its own Makefile, startup code, linker script, layered architecture, vendor sources, and documentation.

## Design Principles

- Procedural bare-metal C
- CMSIS for Cortex-M3 and STM32F103 device support
- STM32F10x SPL for peripheral access
- No STM32 HAL, LL, libopencm3, Arduino Core, or RTOS
- Strict downward dependencies between architecture layers
- Non-blocking application logic whenever practical
- Hardware-independent Application and Service interfaces
- Independently buildable and flashable examples

## Available Examples

| Directory | Description | Main components |
|---|---|---|
| [`01-blink-led`](01-blink-led) | Non-blocking blink of the Blue Pill PC13 status LED | GPIO, SysTick, Time Service, Indication Service |

Possible future examples can follow the same numbered convention:

```text
02-gpio-input-interrupt/
03-uart-polling/
04-uart-interrupt-ring-buffer/
05-timer-pwm/
06-spi-display/
07-i2c-sensor/
08-adc-dma/
09-can-loopback/
```

## Clone the Examples Branch

```bash
git clone \
    --branch examples \
    --single-branch \
    https://github.com/haikevins/stm32f1-spl-procedural-baremetal.git \
    stm32f1-spl-examples

cd stm32f1-spl-examples
```

## Build an Example

Enter the example directory and run `make`:

```bash
cd 01-blink-led
make
```

Generated files are written to the example's local `build/` directory:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

Use a custom output name when needed:

```bash
make PROJECT=01-blink-led
```

Clean generated files:

```bash
make clean
```

## Flash

Connect the Blue Pill to an ST-Link through SWD:

```bash
make flash
```

Erase the device:

```bash
make erase
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

Depending on the local toolchain, the Makefile uses `arm-none-eabi-gdb` or `gdb-multiarch`.

## Architecture

Every example follows the same runtime dependency direction:

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

The `system/` directory is the composition root. It connects modules and controls initialization, but it must not contain product behavior.

Each project includes an architecture checker:

```bash
make check-layers
```

Application code must not bypass Services by directly including BSP, SPL, CMSIS, or STM32 device headers.

## Creating a New Example

Use the `template` branch as the clean starting point instead of copying unrelated example behavior:

```bash
git clone \
    --branch template \
    --single-branch \
    https://github.com/haikevins/stm32f1-spl-procedural-baremetal.git \
    02-my-example
```

Then:

1. Remove the cloned `.git` directory if the new project will be committed inside the `examples` branch.
2. Select only the required SPL implementation files in `config/modules.mk`.
3. Add board-specific resources to `bsp/bluepill/`.
4. Add external-device drivers to `ecual/` when required.
5. Add hardware-independent Services.
6. Implement the example behavior in the Application layer.
7. Connect initialization in `system/system_init.c`.
8. Document hardware wiring, configuration, and expected behavior in the example README.
9. Run `make check-layers` and `make` before committing.

Example preparation:

```bash
rm -rf 02-my-example/.git
```

## Prerequisites

- GNU Arm Embedded Toolchain
- GNU Make
- Python 3
- OpenOCD
- `arm-none-eabi-gdb` or `gdb-multiarch`
- ST-Link or another OpenOCD-compatible SWD probe
- STM32F103C8T6 Blue Pill board

## License

The examples are distributed under the MIT License. Refer to the `LICENSE` file inside each example where applicable.
