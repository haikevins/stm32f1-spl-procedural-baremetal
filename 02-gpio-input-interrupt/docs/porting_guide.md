# Porting Guide - 02 - GPIO Input Interrupt

## Current Hardware Assumptions


Wire a normally-open push button:

```text
PA0 ---- push button ---- GND
```

The GPIO uses the internal pull-up. Therefore:

```text
released -> PA0 HIGH
pressed  -> PA0 LOW
```

The falling transition is routed to EXTI line 0. The onboard PC13 LED remains
the output indicator.


## Current Configuration Assumptions


| Setting | Value |
|---|---|
| Button pin | PA0 |
| Input mode | internal pull-up |
| EXTI line | EXTI0 |
| Edge | falling |
| IRQ priority | 2 |
| Debounce time | 30 ms |
| Timebase | 1 ms |
| Status LED | PC13 active-low |

The debounce interval is defined in `config/button_config.h`.


## What Should Remain Portable

Try to keep these layers unchanged when moving to another board with equivalent
functionality:

```text
app/
services/
common/
```

For an external-device example, also keep `ecual/` unchanged when the external
device and protocol remain the same.

## What Usually Changes

```text
bsp/bluepill/
config/
config/modules.mk
```

A larger MCU change may also require:

```text
startup/
linker/
third_party/
tools/openocd/
```

## Pin/Peripheral Porting


To move the button to another pin, update the GPIO port/pin, AFIO port source,
EXTI line, IRQ vector, and priority in `board_pins.h`. If the new pin shares an
EXTI group handler such as EXTI9_5, the BSP ISR must be changed accordingly.


## Clock Review

Never copy prescaler/baud/timer values blindly.

Verify:

- `SystemCoreClock`;
- PCLK1 and PCLK2;
- APB timer ×2 rule;
- selected peripheral bus;
- generated baud/sample/PWM/bus frequency;
- timeout assumptions.

## Interrupt Review

If the peripheral or pin changes:

- verify IRQ vector name;
- verify EXTI line grouping if relevant;
- verify NVIC priority;
- verify pending flag clear sequence;
- verify the startup table contains the correct handler symbol.

## Electrical Review

Check:

- logic voltage;
- common ground;
- pull-up/pull-down requirements;
- current limiting;
- analog input range;
- external-device power-up timing;
- bus line direction.

## Port Validation Checklist

- [ ] New hardware mapping is documented.
- [ ] `config/modules.mk` contains required SPL source files.
- [ ] `make check-layers` passes.
- [ ] Firmware builds with no new architecture exceptions.
- [ ] Peripheral initialization succeeds.
- [ ] Expected observable behavior is reproduced.
- [ ] GDB diagnostics show expected internal state.
- [ ] Failure cases are still bounded and recoverable as designed.
