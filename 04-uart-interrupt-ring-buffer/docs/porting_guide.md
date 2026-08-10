# Porting Guide - 04 - UART Interrupt + Ring Buffer

## Current Hardware Assumptions


The wiring and terminal settings are the same as Example 03:

```text
PA9  USART1_TX  ---> USB-UART RX
PA10 USART1_RX  <--- USB-UART TX
GND              --- common GND
```

Use `115200 8N1`, no hardware flow control.


## Current Configuration Assumptions


| Setting | Value |
|---|---|
| USART | USART1 |
| Baud | 115200 |
| IRQ priority | 5 |
| RX storage | 128 bytes |
| TX storage | 128 bytes |

The `byte_ring_buffer` implementation uses a head/tail design in which one
storage slot is reserved to distinguish full from empty. With 128 storage
bytes, usable capacity is 127 bytes per ring.


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


When moving to another USART, update the physical mapping and IRQ name in the
BSP. Verify the handler name exactly matches the startup vector. Recalculate IRQ
priority policy if the project adds other real-time interrupt sources.


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
