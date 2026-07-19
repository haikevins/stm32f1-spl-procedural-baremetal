# STM32F1 SPL Procedural Bare-Metal Template

A buildable STM32F103C8T6 project skeleton using CMSIS, the STM32F10x
Standard Peripheral Library, GNU Make, and a strict layered architecture.

The template contains no peripheral example and no product logic. It boots,
initializes the empty board and application hooks, then enters a super-loop
that executes `application_process()` and `WFI`.

## Dependency direction

```text
Application -> Services -> BSP / ECUAL -> SPL -> CMSIS -> Hardware
```

The `system/` directory is the composition root. Run `make check-layers` to
reject forbidden include dependencies.

## Vendor sources

Keep the repository's existing `third_party/CMSIS` and
`third_party/STM32F10x_StdPeriph_Driver` directories. They are not duplicated
inside this rewrite bundle.

## Build

```bash
make
```

## Flash

```bash
make flash
```

## Debug

Terminal 1:

```bash
make debug-server
```

Terminal 2:

```bash
make debug
```

## Starting a project

Add modules in this order:

1. SPL source selection in `config/modules.mk`
2. Board resources in `bsp/bluepill/`
3. External-device drivers in `ecual/`
4. Hardware-independent APIs in `services/`
5. Product behavior in `app/`
6. Initialization wiring in `system/system_init.c`

See `docs/architecture.md` and `docs/adding_a_module.md`.
