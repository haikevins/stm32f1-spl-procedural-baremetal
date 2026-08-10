# Porting Guide - 03 - UART Polling

## Current Hardware Assumptions


Use a 3.3 V USB-to-UART adapter:

```text
Blue Pill PA9  USART1_TX  ---> adapter RX
Blue Pill PA10 USART1_RX  <--- adapter TX
Blue Pill GND              --- adapter GND
```

Terminal settings:

```text
115200 baud
8 data bits
no parity
1 stop bit
no flow control
```


## Current Configuration Assumptions


`BOARD_UART_BAUD_RATE` is `115200`.

Board mapping:

```text
USART1_TX = PA9
USART1_RX = PA10
```

TX uses alternate-function push-pull at 50 MHz. RX is floating input.


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


To move the console, update the USART instance, RCC clocks, GPIO port, and TX/RX
pins in the BSP. If moving to USART2/USART3, also verify APB clock selection and
alternate-function pin mapping.


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
