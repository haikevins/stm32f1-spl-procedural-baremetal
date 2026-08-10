# Architecture - 06 - I2C SSD1306 Display

## Runtime Dependency Direction

```text
Application
    |
    v
Services
    |
    +------> BSP
    |
    +------> ECUAL, when an external device exists
                  |
                  v
          Board peripheral APIs
                  |
                  v
            STM32F10x SPL
                  |
                  v
              CMSIS/MCU
```

`system/` is the composition root and owns initialization order.

## Example-Specific Data Flow


The Application renders:

```text
STM32F103
I2C SSD1306

UPTIME <seconds>
SECONDS

[progress bar]
```

Every 100 ms it updates the progress percentage, rebuilds the framebuffer, and
calls `display_service_present()`.

The SSD1306 ECUAL keeps a 1024-byte framebuffer arranged as 8 pages × 128
columns. Pixel, text, and progress-bar functions only manipulate RAM. The
`update` operation sends the display address window followed by framebuffer
data.


## Module Responsibilities

### Application

Owns demo/product policy. It must not know physical pins, peripheral instances,
SPL structures, or interrupt flags.

### Services

Translate board/external-device capabilities into stable application-facing
APIs. Service logic is where debounce, filtering, aggregation, or logical
indications belong.

### BSP

Owns the Blue Pill mapping, clock enable, GPIO configuration, STM32 peripheral
initialization, NVIC setup, and low-level ISR when applicable.

### ECUAL

Used only when this example communicates with an off-chip device. ECUAL owns
the external device protocol and should depend on a board bus abstraction.

### Common

Contains portable helpers or shared types with no STM32 dependency.

### System

Initializes modules in dependency order and then runs the super-loop. It must
not contain the example's behavior.

## Initialization


Board initialization starts the timebase first because I2C polling and OLED
power-on delay depend on it.

```text
board_init()
    |
    +--> board_timebase_init()
    +--> board_display_bus_init()
            |
            +--> GPIOB clock
            +--> PB6/PB7 AF open-drain
            +--> I2C1 clock
            +--> I2C initialization
            +--> wait for bus idle

display_service_init()
    |
    +--> ssd1306_init()
            |
            +--> wait power-on delay
            +--> send SSD1306 init commands
            +--> clear framebuffer
            +--> transfer framebuffer
```


## Low-Level Ownership


The BSP I2C transaction is polling and bounded:

1. wait until BUSY clears;
2. generate START;
3. wait for master mode;
4. send 7-bit address in transmitter mode;
5. send SSD1306 control byte (`0x00` command or `0x40` data);
6. send payload bytes;
7. wait for final byte transmitted;
8. generate STOP.

BERR, ARLO, AF, OVR, and TIMEOUT flags are checked and cleared on abort.

There is no I2C interrupt handler in this example.


## Interrupt Boundary

The weak startup vector is overridden only by the module that owns the active
interrupt source.

The correct flow is:

```text
hardware interrupt
    |
lowest owning module ISR
    |
static low-level state
    |
normal thread-mode API
    |
Service
    |
Application
```

The wrong flow is:

```text
ISR -> Application callback/state machine
```

## Concurrency Principles

When state is shared between ISR and thread mode:

- keep the shared object static and bounded;
- mark asynchronously changed scalar state `volatile` where appropriate;
- make multi-step read/clear operations atomic with a short critical section;
- never hold interrupts disabled while performing slow peripheral operations;
- make overflow/error behavior explicit.

## Why This Separation Matters


This example introduces a real ECUAL driver. SSD1306 code does not need to know
PB6/PB7 or I2C1. It uses the board display bus API. The Service gives the
Application a display capability rather than exposing SSD1306 commands.


## Dependency Enforcement

Run:

```bash
make check-layers
```

A successful build should not require weakening the checker. If a new include
is rejected, reconsider module placement before adding an exception.
