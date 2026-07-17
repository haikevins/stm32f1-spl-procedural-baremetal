# STM32F1 SPL Procedural Bare-Metal Examples

Each directory is a complete project created from the repository's `template`
branch. The common architecture and base interfaces remain consistent so the
differences between examples are limited to the feature being demonstrated.

## Examples

| No. | Project | Topic |
|---:|---|---|
| 01 | `01-blink-led` | GPIO output and non-blocking SysTick timing |

## Common workflow

```bash
cd 01-blink-led
make PROJECT=01-blink-led
make clean
```

For a new example, copy or clone the `template` branch, keep `main.c`, `app.h`,
and `bsp.h` unchanged, and add only the required application, BSP, driver, or
system modules.
