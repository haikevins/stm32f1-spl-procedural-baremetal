# Porting Guide - 08 - ADC + DMA

## Current Hardware Assumptions


Use a potentiometer:

```text
3.3 V ---- potentiometer ---- GND
                  |
                  +---- PA0 / ADC1_IN0
```

The onboard PC13 LED is used as a threshold indicator.

Do not intentionally drive the analog input outside the MCU supply range.


## Current Configuration Assumptions


| Setting | Value |
|---|---|
| ADC | ADC1 |
| Channel | 0 |
| Pin | PA0 |
| ADC reference assumption | 3300 mV |
| Raw full scale | 4095 |
| ADC sample time | 55.5 cycles |
| ADC clock configuration | PCLK2 / 6 |
| Trigger timer | TIM3 |
| Timer tick target | 1 MHz |
| Sample rate | 1 kHz |
| DMA | DMA1 Channel 1 |
| DMA mode | circular |
| DMA storage | 64 halfwords |
| Published block | 32 samples |
| IRQ priority | preemption 1, subpriority 0 |
| LED ON threshold | 1800 mV |
| LED OFF threshold | 1500 mV |

The two LED thresholds provide hysteresis.


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


When changing the ADC input, update GPIO pin and ADC channel together. When
changing sample rate, verify TIM3 clock, timer tick divisibility, period range,
ADC conversion time, and DMA processing budget. If VDDA differs from 3.3 V,
update or calibrate the reference used for millivolt conversion.


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
