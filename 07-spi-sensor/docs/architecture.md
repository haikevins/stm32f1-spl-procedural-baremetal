# Layered Architecture

## Runtime dependency direction

```text
Application
    |
    v
Services
    |
    v
ECU Abstraction
    |
    v
BSP
    |
    v
STM32F10x Standard Peripheral Library
    |
    v
CMSIS
    |
    v
STM32F103 hardware
```

`system/` is the composition root. It initializes the board, services,
external-device driver, and Application in dependency order.

## Example 06 dependency paths

```text
Application
    |
    v
Display Service
    |
    v
SSD1306 ECUAL
    |
    v
Board Display Bus
    |
    v
SPI1 / GPIO
```

```text
Application -> Time Service -> Board Timebase -> SysTick
```

## Layer responsibilities

### Application

Owns the elapsed-time display and progress animation. It sees only Service
interfaces and hardware-independent configuration.

### Services

Expose display and time capabilities without revealing the SSD1306,
STM32F103 pins, SPI registers, SPL, or CMSIS.

### ECU Abstraction

The SSD1306 driver contains controller commands, a fixed 1024-byte
framebuffer, glyph rendering, and pixel primitives. It depends on the BSP
display-bus interface rather than STM32 headers.

### BSP

Maps the display bus to SPI1 and PA4/PA5/PA7/PB0/PB1. It owns GPIO, SPI
configuration, reset timing, and bounded peripheral polling.

### Common

Contains reusable hardware-independent helpers and types.

### System

Owns initialization order, the super-loop, idle policy, and fatal-error
policy. It contains no display-demo behavior.

## Interrupt rule

Only SysTick is used. Its handler remains in the BSP timebase module and
increments a low-level millisecond counter.

SPI transfers are polling-based in this example, so there is no SPI ISR.

## Enforcement

Run:

```bash
make check-layers
```

The checker rejects forbidden source-level dependencies before compilation.
