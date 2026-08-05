# Example 02: GPIO Input Interrupt

A layered STM32F103C8T6 Blue Pill example using CMSIS and the STM32F10x
Standard Peripheral Library.

An external push button on PA0 generates an EXTI0 interrupt. After a
non-blocking 30 ms debounce interval, each valid press toggles the onboard
active-low LED on PC13.

## Hardware wiring

```text
Blue Pill PA0 ---- push button ---- GND
```

PA0 uses the internal pull-up resistor, so no external pull-up resistor is
required.

| Resource | Configuration |
|---|---|
| Status LED | PC13, active-low output |
| Push button | PA0, internal pull-up |
| EXTI line | EXTI0 |
| Interrupt trigger | Falling edge |
| Debounce time | 30 ms |
| Timebase | SysTick at 1 kHz |

## Expected behavior

1. The LED starts off.
2. Press and release the PA0 button.
3. Each valid press toggles the PC13 LED once.
4. Contact bounce does not cause multiple LED toggles.

## Dependency paths

```text
Application
    -> Button Service
        -> Board Button
            -> GPIO / AFIO / EXTI / NVIC

Application
    -> Indication Service
        -> Board LED
            -> GPIO

Button Service
    -> Time Service
        -> Board Timebase
            -> SysTick
```

Application code does not include BSP, SPL, CMSIS, or STM32 device headers.

## Interrupt policy

`EXTI0_IRQHandler()` belongs to the BSP module that owns the physical button.
The ISR only:

- checks the EXTI pending flag;
- records a low-level edge event;
- clears the EXTI pending flag.

Debouncing, button validation, and LED control run outside interrupt context.

## Build

```bash
make
```

The build first checks the project dependency rules:

```bash
make check-layers
```

## Flash

```bash
make flash
```

The example deliberately keeps `system_idle()` on `__NOP()` rather than
`__WFI()` so debug probes without an NRST connection can attach reliably
after power-up.

## Debug

Terminal 1:

```bash
make debug-server
```

Terminal 2:

```bash
make debug
```

## Configuration

- Debounce interval: `config/button_config.h`
- Timebase frequency: `config/board_config.h`
- Board pin mapping: `bsp/bluepill/src/board_pins.h`
- Selected SPL modules: `config/modules.mk`

## Selected SPL modules

```text
stm32f10x_exti.c
stm32f10x_gpio.c
stm32f10x_rcc.c
```

## License

This example is licensed under the MIT License.
