# Porting Guide - 07 - SPI W25Q64 Memory

## Current Hardware Assumptions


Wire the six-pin module:

```text
STM32F103C8T6       W25Q64
--------------------------------
3.3V        ------  VCC
GND         ------  GND
PA4         ------  CS
PA5         ------  CLK
PA6         ------  D1 / DO / MISO
PA7         ------  D0 / DI / MOSI
```

Use 3.3 V supply and logic.


## Current Configuration Assumptions


| Setting | Value |
|---|---|
| SPI | SPI1 |
| CS | PA4 |
| SCK | PA5 |
| MISO | PA6 |
| MOSI | PA7 |
| SPI mode | 0 |
| Maximum requested SPI clock | 5 MHz |
| SPI polling timeout | 20 ms |
| Power-on delay | 10 ms |
| Page-program timeout | 50 ms |
| Sector-erase timeout | 2000 ms |
| Test sector | 0x007FF000 |
| Test length | 32 bytes |
| Heartbeat | 500 ms |

With the normal 72 MHz PCLK2, the BSP selects `/16`, producing 4.5 MHz.


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


To use another SPI instance, update SCK/MISO/MOSI/CS mapping and the APB clock
used for prescaler selection. To support another NOR geometry, review size,
page size, sector size, JEDEC validation, address width, command set, and
timeouts together.


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
