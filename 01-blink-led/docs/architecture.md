# Architecture

## Dependency direction

```text
app -> bsp -> STM32F10x SPL -> CMSIS device -> STM32F103 hardware
  \
   -> system_time -> CMSIS Cortex-M3 SysTick
```

Higher layers may depend on lower layers. Board-specific code must not depend on application code.

## Responsibilities

### `app/`

Owns the product behavior:

- initialize the services needed by the example;
- decide the 500 ms blink period;
- call BSP APIs rather than accessing GPIO registers directly.

### `bsp/`

Owns board-specific details:

- onboard LED port and pin;
- active-low polarity;
- RCC clock enable and GPIO setup through SPL.

### `system/`

Owns MCU runtime support:

- reset/startup sequence;
- exception and interrupt handlers;
- SysTick-based millisecond time base;
- SPL configuration header.

### `third_party/`

Contains vendor code. The example only compiles the required SPL modules:

- `stm32f10x_gpio.c`;
- `stm32f10x_rcc.c`;
- `system_stm32f10x.c` from CMSIS device support.

### `linker/`

Defines the STM32F103C8 memory map and places the vector table, code, initialized data, zero-initialized data, heap reservation, and stack reservation.

### `scripts/`

Contains OpenOCD and debug helpers. The default configuration targets an ST-LINK/V2-style four-wire probe through the native `stlink-dap.cfg` DAP driver. Because the probe has no `NRST` signal, flash recovery uses BOOT0/System Memory and the target is reset manually after programming. Hardware access remains outside application code.

## Runtime sequence

1. The Cortex-M3 loads `_estack` and `Reset_Handler` from the vector table.
2. `Reset_Handler` copies `.data` from Flash to SRAM.
3. `Reset_Handler` clears `.bss`.
4. `SystemInit()` configures clocks and the vector table base.
5. `main()` initializes BSP and SysTick.
6. `SysTick_Handler()` increments a 32-bit millisecond counter.
7. `App_Run()` compares elapsed unsigned time and toggles the LED every 500 ms.

## Why the application loop is non-blocking

A calibrated busy loop changes duration when clock frequency or compiler optimization changes. The SysTick design makes the timing explicit and leaves the main loop available for future button, UART, or event-processing work.
