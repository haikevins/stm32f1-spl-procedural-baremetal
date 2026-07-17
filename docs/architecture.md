# Architecture

## Dependency direction

```text
main.c
  -> bsp
  -> app

app
  -> bsp
  -> drivers
  -> lib
  -> middleware

bsp
  -> CMSIS / SPL

drivers
  -> bsp or narrow hardware interfaces
  -> CMSIS / SPL when appropriate
```

Lower layers must not depend on application policy.

## Stable template interfaces

The following interfaces form the common base for every example:

```c
void BSP_Init(void);
void App_Init(void);
void App_Run(void);
```

`main.c`, `app.h`, and `bsp.h` should normally remain byte-for-byte identical to
the template. Concrete examples implement behavior in `app.c`, extend `bsp.c`,
and add focused modules such as `bsp_led.c` or `system_time.c`.

## Startup sequence

1. `Reset_Handler` initializes `.data` and `.bss`.
2. `SystemInit()` configures the MCU clock before `main()`.
3. `main()` calls `BSP_Init()` once.
4. `main()` calls `App_Init()` once.
5. `main()` repeatedly calls non-blocking `App_Run()`.

## Layer responsibilities

### `app/`

Application state, policies, use-case orchestration, and the super-loop step.

### `bsp/`

Board-specific pins, onboard peripherals, and board initialization.

### `drivers/`

Reusable drivers for external devices and sensors.

### `lib/`

Hardware-independent helpers and algorithms.

### `middleware/`

Protocol stacks, RTOS integration, file systems, and communication services.

### `system/`

Startup assembly, exception handlers, system services, and low-level runtime
support.
