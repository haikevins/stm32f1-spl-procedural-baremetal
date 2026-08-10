# Porting Guide - 01 - Blink LED

## Current Hardware Assumptions


The example uses the Blue Pill onboard LED connected to PC13. The board layer
defines it as active-low:

```text
PC13 LOW  -> LED ON
PC13 HIGH -> LED OFF
```

No external components are required.


## Current Configuration Assumptions


| Setting | Source | Value |
|---|---|---|
| Status LED | `board_pins.h` | PC13, active-low |
| Timebase | `board_config.h` | 1000 Hz |
| Application toggle period | `application_config.h` | 500 ms |

`board_led_init()` presets the inactive output level before configuring the pin
as push-pull output, then `indication_service_init()` explicitly requests the
logical OFF state.


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


To move the LED, change `bsp/bluepill/src/board_pins.h` and, if necessary, the
GPIO clock. To change the timebase frequency, update `BOARD_TIMEBASE_HZ`, but
keep the Service's millisecond semantics consistent or update the Service API
accordingly.


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
