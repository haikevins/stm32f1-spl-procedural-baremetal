# STM32F1 SPL Procedural Bare-Metal Template

A minimal C project template for the STM32F103C8Tx using CMSIS, the
STM32F10x Standard Peripheral Library (SPL), GNU Make, and the GNU Arm
Embedded Toolchain.

The `template` branch contains only reusable project infrastructure and neutral
extension points. Complete applications belong in the `examples` branch.

## Project structure

```text
.
├── app/
│   ├── inc/app.h
│   └── src/
│       ├── app.c
│       └── main.c
├── bsp/
│   ├── inc/bsp.h
│   └── src/bsp.c
├── drivers/
│   ├── inc/.gitkeep
│   └── src/.gitkeep
├── lib/
│   ├── inc/.gitkeep
│   └── src/.gitkeep
├── middleware/
│   ├── inc/.gitkeep
│   └── src/.gitkeep
├── system/
├── third_party/
├── linker/
├── scripts/
├── docs/
├── build/.gitkeep
├── Makefile
├── LICENSE
└── .gitignore
```

## Core flow

```text
Reset_Handler
    -> SystemInit()
    -> main()
        -> BSP_Init()
        -> App_Init()
        -> while (1)
            -> App_Run()
```

Responsibilities:

- `main.c` owns only the top-level initialization order and super loop.
- `bsp.c` initializes board-level resources.
- `app.c` contains application policy and non-blocking processing.
- `drivers/` contains reusable external-device drivers.
- `lib/` contains hardware-independent utilities.
- `middleware/` contains protocol stacks, RTOS ports, and file systems.

## Requirements

Install GNU Make and the GNU Arm Embedded Toolchain so these commands are in
`PATH`:

```text
arm-none-eabi-gcc
arm-none-eabi-objcopy
arm-none-eabi-size
```

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

To choose another output name:

```bash
make PROJECT=my_firmware
```

Remove generated files while preserving `build/.gitkeep`:

```bash
make clean
```

The Makefile intentionally exposes only the normal build and clean workflows.
Flashing and debugging may be run with the scripts under `scripts/` or added by
a concrete project when needed.

## Flash

Flash the firmware with ST-Link and OpenOCD:

```bash
make flash
```

## Creating an example

Clone the template branch and rename the directory:

```bash
git clone --branch template --single-branch \
  git@github.com:haikevins/stm32f1-spl-procedural-baremetal.git \
  02-example-name
```

Keep the base interfaces unchanged where possible:

```c
void BSP_Init(void);
void App_Init(void);
void App_Run(void);
```

Then add only the modules and implementation required by the example. A normal
example should keep `main.c`, `app.h`, and `bsp.h` identical to the template.

## Design rules

- Keep `main.c` small and unchanged across projects.
- Call `BSP_Init()` exactly once from `main()`.
- Keep `App_Run()` non-blocking.
- Put application decisions in `app/`, not in BSP or drivers.
- Put board pin mappings and board peripheral setup in `bsp/`.
- Do not commit generated files from `build/`.
- Keep vendor code under `third_party/` unchanged.
