# Porting Guide - 06 - I2C SSD1306 Display

## Current Hardware Assumptions


Connect the four-pin OLED:

```text
Blue Pill      OLED
-------------------
GND        --> GND
3.3V       --> VCC
PB6        --> SCL
PB7        --> SDA
```

The example uses I2C1. The default 7-bit display address is `0x3C`.

Most common four-pin modules include pull-up resistors. If yours does not, SCL
and SDA require pull-ups to 3.3 V.


## Current Configuration Assumptions


| Setting | Value |
|---|---|
| I2C peripheral | I2C1 |
| SCL | PB6 |
| SDA | PB7 |
| Address | 0x3C |
| I2C clock | 400 kHz |
| I2C transaction timeout | 20 ms |
| OLED power-on delay | 100 ms |
| Timebase | 1 ms |
| Display | 128x64 |
| Framebuffer | 1024 bytes |
| Demo update | 100 ms |
| Progress step | 2% |

Change the address to `0x3D` in `board_config.h` only if the actual module uses
that address.


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


To move I2C pins or the peripheral, update the BSP mapping and bus
initialization. To use another OLED controller, keep the board I2C bus and
replace the ECUAL driver. To change display dimensions, framebuffer layout and
rendering bounds must be reviewed together.


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
