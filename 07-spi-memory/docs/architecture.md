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

`system/` is the composition root.

## Example 07 dependency paths

```text
Application
    |
    v
Memory Service
    |
    v
W25Q64 ECUAL Driver
    |
    v
Board Memory Bus
    |
    v
GPIO / SPI1
```

```text
Application -> Indication Service -> Board LED -> GPIO
Application -> Time Service -> Board Timebase -> SysTick
```

## Responsibilities

### Application

Runs one destructive erase/program/read-back self-test in the configured
sector and publishes GDB-visible diagnostic variables. It never accesses
SPL, GPIO, SPI, or W25Q64 command constants directly.

### Services

Expose memory operations, status LED behavior, and time without revealing
STM32 implementation details.

### ECU Abstraction

The W25Q64 driver owns JEDEC-ID, status-register, read, page-program, sector
erase, 24-bit address encoding, WEL checks, BUSY polling, page boundaries,
and device-size validation.

### BSP

Maps the memory bus to SPI1 and PA4/PA5/PA6/PA7. It owns GPIO direction,
chip-select timing, SPI mode/speed, and bounded SPI flag polling.

### Interrupt rule

SPI transfers are polling-based. Only SysTick is used for the millisecond
timebase. There is no SPI interrupt handler.

## Enforcement

```bash
make check-layers
```
