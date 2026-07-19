# Example 01: Non-Blocking LED Blink

A layered STM32F103C8T6 Blue Pill example using CMSIS and the STM32F10x
Standard Peripheral Library.

The onboard active-low LED on PC13 toggles every 500 ms. SysTick provides a
1 kHz timebase, and the super-loop sleeps with `WFI` between interrupts.

## Dependency paths

```text
Application -> Time Service -> Board Timebase -> CMSIS SysTick
Application -> Indication Service -> Board LED -> STM32 SPL GPIO
```

Application code does not include BSP, SPL, CMSIS, or STM32 device headers.

## Build

Preserve the existing `third_party/` vendor sources, then run:

```bash
make
```

## Flash

```bash
make flash
```

## Debug

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

## Configuration

- Blink period: `config/application_config.h`
- Timebase frequency: `config/board_config.h`
- Board pin mapping: `bsp/bluepill/src/board_pins.h`
- Selected SPL modules: `config/modules.mk`

## Expected behavior

```text
PC13 low  -> LED on
PC13 high -> LED off
Toggle period: 500 ms
```
