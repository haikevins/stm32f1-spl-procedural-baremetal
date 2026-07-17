# 01 - Blink LED

A non-blocking Blink LED example created directly from the `template` branch.
It targets a common STM32F103C8 Blue Pill board whose onboard LED is connected
to PC13 and is active-low.

## What changed from the template

The base project was copied without redesigning its core flow:

- `app/inc/app.h` is unchanged;
- `app/src/main.c` is unchanged;
- `bsp/inc/bsp.h` is unchanged;
- `app/src/app.c` implements the blink policy;
- `bsp/src/bsp.c` extends board initialization;
- `bsp_led.*` adds the onboard LED BSP module;
- `system_time.*` and `SysTick_Handler()` add a 1 ms time base.

```text
main()
    -> BSP_Init()
        -> SystemCoreClockUpdate()
        -> BSP_LED_Init()
    -> App_Init()
        -> System_Time_Init()
    -> while (1)
        -> App_Run()
            -> BSP_LED_Toggle() every 500 ms
```

## Build

```bash
make PROJECT=01-blink-led
```

Generated files:

```text
build/01-blink-led.elf
build/01-blink-led.hex
build/01-blink-led.bin
build/01-blink-led.map
```

Clean the project:

```bash
make clean
```

The example deliberately uses the same minimal Makefile as the template. It has
no separate `size`, `disasm`, `rebuild`, or `help` target. Firmware size is
printed automatically after linking.

## Relevant files

```text
app/src/app.c
bsp/inc/bsp_led.h
bsp/src/bsp_led.c
system/inc/system_time.h
system/src/system_time.c
system/src/stm32f10x_it.c
```
