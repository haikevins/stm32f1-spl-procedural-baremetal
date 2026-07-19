# Layered Architecture

## Runtime dependency direction

```text
Application
    |
    v
Services
    |
    v
BSP / ECU Abstraction
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

`system/` is the composition root. It may initialize and connect multiple
layers, but it must not contain product behavior.

`startup/`, `linker/`, `runtime/`, `config/`, `tools/`, and `third_party/`
are infrastructure areas rather than application layers.

## Layer responsibilities

### Application

Contains product policy, state machines, and non-blocking behavior.

Application code may include Services and hardware-independent Common code.
It must not include BSP, ECUAL, CMSIS, SPL, or raw STM32 headers.

### Services

Expose hardware-independent capabilities such as time, indications,
communication, diagnostics, scheduling, and event delivery.

Services may use BSP and ECUAL public APIs. Services must not include
Application headers or raw STM32/SPL headers.

### BSP

Maps logical board resources to physical MCU pins and peripherals.

Examples include onboard LEDs, buttons, console ports, and the board
timebase. BSP modules may call SPL and CMSIS.

### ECU Abstraction

Contains drivers for external devices such as displays, sensors, EEPROMs,
and transceivers. For portability, ECUAL modules should use BSP bus
interfaces rather than including STM32 SPL directly.

### Common

Contains hardware-independent utilities such as CRC, fixed-size queues,
ring buffers, bit utilities, and generic data types.

### System

Owns the composition root, initialization order, the main super-loop, idle
policy, and fatal-error policy. It may connect layers but must not implement
application behavior.

### Vendor peripheral layer

`third_party/STM32F10x_StdPeriph_Driver` acts as the vendor peripheral
driver or MCAL-equivalent layer for this SPL-based project.

`third_party/CMSIS` provides Cortex-M3 and STM32F103 device definitions.

## Interrupt rule

An ISR must remain in the lowest layer that owns its hardware resource. It
may acknowledge flags, move data into a static low-level buffer, or update a
low-level counter.

An ISR must not call Application or Service functions.

## Enforcement

Run:

```bash
make check-layers
```

The checker rejects forbidden project-header dependencies before the
firmware is compiled.
