# Porting Guide - 05 - Timer PWM

## Current Hardware Assumptions


Use an external LED:

```text
PA0 / TIM2_CH1 ---- 330 ohm ---- LED anode
GND --------------------------- LED cathode
```

The onboard PC13 LED is not used for PWM.


## Current Configuration Assumptions


| Setting | Value |
|---|---|
| PWM timer | TIM2 |
| Channel | CH1 |
| Output pin | PA0 |
| Timer target tick | 1 MHz |
| PWM frequency | 1 kHz |
| SysTick timebase | 1 kHz |
| Duty unit | permille, 0..1000 |
| Duty update period | 10 ms |
| Duty step | 10 permille |

The ramp therefore takes roughly 1 second from 0 to 100% and another second
back to 0%.


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


To use another PWM pin, select a timer/channel that is actually mapped to that
pin on STM32F103 and update both GPIO and timer definitions. Re-check timer bus
(APB1 vs APB2) and the ×2 timer clock rule.


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
