# STM32F1 SPL Procedural Bare-Metal Template

A minimal, buildable C project template for the STM32F103C8Tx using CMSIS and
the STM32F10x Standard Peripheral Library (SPL).

This branch is intentionally feature-neutral. It provides project structure,
startup code, linker configuration, build tooling, and empty extension points.
Concrete projects such as LED, button, UART, timer, and communication examples
belong in the `examples` branch.

## Target

- MCU: STM32F103C8Tx
- Core: Arm Cortex-M3
- Flash: 64 KiB
- SRAM: 20 KiB
- Library: STM32F10x SPL v3.5.0
- Build system: GNU Make
- Debug probe: ST-Link via OpenOCD

## Structure

```text
.
├── app/                          # Application flow and product logic
│   ├── inc/app.h
│   └── src/
│       ├── app.c
│       └── main.c
├── bsp/                          # Minimal board support layer
│   ├── inc/bsp.h
│   └── src/bsp.c
├── drivers/                      # External-device drivers
│   ├── inc/.gitkeep
│   └── src/.gitkeep
├── lib/                          # Hardware-independent utilities
│   ├── inc/.gitkeep
│   └── src/.gitkeep
├── middleware/                   # Protocol stacks, RTOS, file systems
│   ├── inc/.gitkeep
│   └── src/.gitkeep
├── system/                       # Startup, interrupts, config, syscalls
│   ├── inc/
│   │   ├── stm32f10x_conf.h
│   │   └── stm32f10x_it.h
│   ├── src/
│   │   ├── stm32f10x_it.c
│   │   └── syscalls.c
│   └── startup/startup_stm32f10x_md_gcc.s
├── third_party/                  # CMSIS and STM32F10x SPL
├── linker/STM32F103C8Tx_FLASH.ld
├── scripts/
│   ├── debug.sh
│   ├── flash.sh
│   └── openocd.cfg
├── docs/architecture.md
├── build/.gitkeep
├── Makefile
├── LICENSE
└── .gitignore
```

## Requirements

Install the following tools and ensure they are available in `PATH`:

- `arm-none-eabi-gcc`
- `arm-none-eabi-objcopy`
- `arm-none-eabi-size`
- GNU Make
- OpenOCD for flashing and debugging
- `arm-none-eabi-gdb` for debugging

On Debian/Ubuntu, package names commonly include:

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi make openocd gdb-multiarch
```

Package names vary by distribution.

## Build

```bash
make
```

Generated files:

```text
build/firmware.elf
build/firmware.hex
build/firmware.bin
build/firmware.map
```

Use a custom output name:

```bash
make PROJECT=my_firmware
```

Clean generated files while preserving `build/.gitkeep`:

```bash
make clean
```

## Flash

Connect an ST-Link probe over SWD, then run:

```bash
make flash
```

Equivalent direct command:

```bash
./scripts/flash.sh build/firmware.elf
```

## Debug

```bash
make debug
```

The script starts OpenOCD and opens a GDB session connected to
`localhost:3333`.

## Start a new project

Clone the template branch:

```bash
git clone --branch template --single-branch \
  git@github.com:haikevins/stm32f1-spl-procedural-baremetal.git \
  my-stm32-project
```

Then remove the inherited repository history and initialize a new repository:

```bash
cd my-stm32-project
rm -rf .git
git init
```

Recommended first changes:

1. Set `PROJECT` in the Makefile or pass it to `make`.
2. Add board initialization to `BSP_Init()`.
3. Add reusable hardware modules under `drivers/`.
4. Add application behavior to `App_Init()` and `App_Run()`.
5. Add interrupt handlers only when required.

## Design rules

- Keep `main.c` small.
- Keep application policy out of BSP and drivers.
- Keep `App_Run()` non-blocking.
- Do not commit generated build files.
- Do not modify `third_party/` without documenting the reason.
- Put complete demonstrations in the `examples` branch, not in this template.

See [docs/architecture.md](docs/architecture.md) for the dependency rules and
startup flow.

## License

Project-owned files are licensed under the MIT License. Files under
`third_party/` retain their original STMicroelectronics licenses and notices.
