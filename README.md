# STM32F103 SPL Procedural Bare-Metal Template

## 1. When to Use This Template

Use this template when starting a small STM32F103C8T6 project that should use
CMSIS + STM32F10x SPL while keeping:

- explicit startup;
- explicit linker script;
- explicit module initialization;
- downward architecture dependencies;
- non-blocking super-loop logic;
- no HAL/RTOS framework ownership.

It is a skeleton, not a completed peripheral demo.

## 2. Current Target

| Item | Value |
|---|---|
| MCU | STM32F103C8T6 |
| CPU | Cortex-M3 |
| Device class | STM32F10X_MD |
| HSE definition | 8 MHz |
| Build | GNU Make |
| Language | C11 |
| Peripheral layer | STM32F10x SPL |
| Core/device layer | CMSIS |

## 3. Directory Structure

```text
app/
services/
ecual/
bsp/bluepill/
common/
config/
system/
platform/
runtime/
startup/
linker/
third_party/
tests/
tools/
docs/
```

The directory names encode architecture responsibility, not only code
organization.

## 4. Target Architecture

```text
Application
    |
    v
Services
    |
    +------> BSP
    |
    +------> ECUAL
                  |
                  v
         board bus interfaces
                  |
                  v
         STM32F10x SPL
                  |
                  v
               CMSIS
                  |
                  v
            STM32F103
```

`system/` is the composition root.

## 5. Dependency Rules

### Application

May depend on Services, Common, and configuration relevant to product policy.

Must not include BSP, ECUAL, SPL, or CMSIS headers.

### Services

May depend on BSP, ECUAL, Common, and configuration.

Must not include Application or raw SPL/CMSIS peripheral headers.

### BSP

Owns board pins, peripheral instances, RCC setup, NVIC setup, and low-level
interrupts.

May use SPL and CMSIS.

### ECUAL

Owns external-device protocol logic.

Prefer depending on board bus APIs rather than directly on SPL.

### SPL

Acts as the peripheral-driver/MCAL-equivalent layer in this repository.

### Platform

Contains platform-specific helpers that are not naturally board resources.
Platform code may use CMSIS/SPL when appropriate.

## 6. Layer Checker

Run:

```bash
make check-layers
```

The checker inspects project includes and rejects forbidden upward
dependencies.

It is intentionally simple and source-oriented; it does not replace design
review.

## 7. Startup Sequence

The startup file provides the vector table and reset handler.

```text
Reset_Handler
    |
    +--> copy .data
    +--> clear .bss
    +--> SystemInit()
    +--> main()
```

Unused handlers are weak aliases of `Default_Handler`.

A module takes ownership of an interrupt by providing a strong handler with the
exact vector name.

## 8. Runtime Initialization

The default template uses:

```text
main()
    |
    +--> system_init()
            |
            +--> board_init()
            +--> application_init()
```

A real project normally inserts Service and external-device initialization
between Board and Application.

## 9. Linker Script

The linker script defines the STM32F103C8T6 flash/SRAM regions and exports
symbols consumed by startup:

```text
_sidata
_sdata
_edata
_sbss
_ebss
_estack
```

When porting to a part with different memory size, update the linker script
before trusting any build.

## 10. Vector Table and Interrupt Extension

The vector table contains Cortex-M3 exceptions plus STM32F103 medium-density
interrupts.

To add an interrupt:

1. initialize the peripheral;
2. clear pending flags;
3. configure priority;
4. enable the NVIC line;
5. implement the strong handler;
6. keep the handler in the lowest owning layer.

## 11. Fault Handling

Fault vectors default to the startup `Default_Handler` unless explicitly
overridden.

Production firmware may add dedicated HardFault/BusFault/UsageFault diagnostics,
but the template intentionally remains minimal.

## 12. `main()` and Composition

`main()` contains no product logic beyond:

```c
if (!system_init())
{
    system_panic();
}

for (;;)
{
    application_process();
    system_idle();
}
```

This keeps startup composition separate from Application behavior.

## 13. Template Idle Behavior

### Using `WFI`

The template currently calls `__WFI()` from `system_idle()` and panic.

This is appropriate when the project has reliable interrupt wake sources and
low-power idle behavior is desired.

### Using `NOP`

The completed examples in this repository use `__NOP()` instead to make SWD
debugging predictable without NRST.

When creating a new project, choose one policy deliberately.

## 14. Cortex-M3 Abstraction

CMSIS provides:

- `__WFI()`;
- `__NOP()`;
- `__disable_irq()`;
- NVIC functions;
- `SysTick_Config()`;
- `SystemCoreClock`.

Higher layers should not use these merely for convenience. Keep architecture
ownership intact.

## 15. SPL/Device Layer Skeleton

SPL is linked explicitly through `config/modules.mk`.

Example:

```make
SPL_SOURCES := \
    third_party/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
    third_party/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c
```

Add only the SPL implementation files required by the project.

## 16. Makefile

The Makefile:

- compiles project sources recursively;
- adds CMSIS system source;
- includes selected SPL sources;
- links with the project linker script;
- generates ELF/HEX/BIN/listing/map artifacts;
- provides OpenOCD and GDB targets;
- runs the layer checker.

## 17. Build Artifacts

A normal build generates:

```text
build/firmware.elf
build/firmware.hex
build/firmware.bin
build/firmware.lst
build/firmware.map
```

## 18. Make Targets

```bash
make
make check-layers
make size
make tree
make flash
make erase
make debug-server
make debug
make clean
```

## 19. OpenOCD

The default configuration uses:

```tcl
source [find interface/stlink.cfg]
transport select hla_swd
source [find target/stm32f1x.cfg]
reset_config none
adapter speed 1000
```

## 20. GDB

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

Use breakpoints first at `main`, `system_init`, and the board initialization
routine before debugging higher-level state.

## 21. Creating a New Project from the Template

Recommended sequence:

1. copy the template;
2. define board resources;
3. add required SPL source files;
4. add BSP initialization;
5. add ECUAL for off-chip devices;
6. add Services;
7. add Application behavior;
8. connect initialization in `system_init()`;
9. add interrupt handoff if required;
10. document wiring and tests;
11. run layer checker;
12. build and hardware-test.

## 22. Completion Checklist

### Architecture

- Application has no BSP/SPL/CMSIS includes.
- Services contain no Application dependency.
- External-device logic is isolated from board wiring.
- ISR ownership is explicit.

### Runtime

- `.data` and `.bss` initialize correctly.
- `system_init()` orders dependencies correctly.
- `application_process()` is bounded.
- panic behavior is intentional.

### Peripheral

- Pin mapping matches schematic.
- RCC clocks are enabled.
- Bus/timer clock assumptions are verified.
- Interrupt flags are cleared correctly.
- Timeout/overflow behavior is defined.

### Tooling

- `make check-layers` passes.
- clean build succeeds.
- OpenOCD connects.
- GDB symbols are usable.

### Docs

- README documents behavior and wiring.
- architecture document explains ownership.
- porting guide describes clock/pin/IRQ changes.

## 23. Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/adding_a_module.md`](docs/adding_a_module.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)

## 24. License

See `LICENSE`.
