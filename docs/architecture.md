# Project Architecture

This template separates application logic from board-specific and vendor code.
It intentionally contains no LED, button, UART, or other feature example.
Feature implementations belong in the `examples` branch or in a project created
from this template.

## Dependency direction

```text
app
 |
 v
bsp --------> drivers
 |                |
 v                v
system         middleware
 |                |
 +-------> third_party

lib may be used by app, bsp, drivers, and middleware.
```

Dependencies should point downward. Lower layers must not depend on application
code.

## Layers

### `app/`

Contains the product-specific program flow. `main.c` initializes the BSP and
application, then repeatedly calls `App_Run()`. Keep `App_Run()` non-blocking so
that the project can later adopt cooperative scheduling or an RTOS without a
large rewrite.

### `bsp/`

Contains board-level initialization and board-specific mappings. The minimal
template only provides `BSP_Init()`. Add modules such as `bsp_led`, `bsp_button`,
or `bsp_uart` only when a concrete board or project requires them.

### `drivers/`

Contains reusable drivers for external devices and sensors. Drivers should use
abstracted hardware services where practical and should not contain application
policy.

### `lib/`

Contains reusable, hardware-independent utilities such as ring buffers, CRC
helpers, state machines, and data structures.

### `middleware/`

Contains protocol stacks, file systems, RTOS integrations, and other components
that sit between drivers and the application.

### `system/`

Contains startup code, the interrupt file, system configuration, and C-library
syscall stubs. `SystemInit()` from CMSIS runs before `main()`.

### `third_party/`

Contains CMSIS and the STM32F10x Standard Peripheral Library. Keep upstream
notices and licenses intact. Avoid modifying these files unless a documented
vendor patch is required.

## Startup flow

```text
Reset
  -> Reset_Handler
      -> initialize .data
      -> clear .bss
      -> SystemInit()
      -> __libc_init_array()
      -> main()
          -> BSP_Init()
          -> App_Init()
          -> while (1)
              -> App_Run()
```

## Adding a feature

1. Add reusable peripheral or sensor code under `drivers/`.
2. Add board pin mappings and board initialization under `bsp/`.
3. Add feature orchestration under `app/`.
4. Add any required interrupt handler in `system/src/stm32f10x_it.c`.
5. Keep generated files inside `build/`.
