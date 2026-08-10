# Architecture - 04 - UART Interrupt + Ring Buffer

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


RX path:

```text
USART RXNE
    |
USART1_IRQHandler
    |
    +--> read DR
    +--> push byte into RX ring
    +--> record error/overflow counters
    |
thread mode
    |
uart_service_try_read_byte()
```

TX path:

```text
Application/Service try_write
    |
push into TX ring
    |
enable TXE interrupt
    |
USART1_IRQHandler
    |
pop TX byte -> DR
    |
ring empty?
    |
    +--> disable TXE interrupt
```

Application uses a fixed processing budget of 32 operations per
`application_process()` call. This prevents one busy UART stream from owning
the super-loop forever.


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


`board_uart_init()`:

1. initializes RX and TX ring objects;
2. clears error/overflow counters;
3. configures PA9/PA10;
4. configures USART1 115200 8N1;
5. assigns and enables the USART1 NVIC interrupt;
6. enables RXNE and USART error interrupts;
7. leaves TXE interrupt disabled until data is queued;
8. enables USART1.

The Service remains a thin hardware-independent wrapper.


## Low-Level Ownership


The ISR reads the USART status register once and handles receive/error state and
TXE state.

Receive error bits counted are:

- ORE;
- NE;
- FE;
- PE.

An RX byte that arrives while the RX ring is full increments the overflow
counter.

TXE interrupt is disabled when the transmit ring becomes empty, preventing an
interrupt storm while TXE remains asserted.


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


This example makes ISR/thread ownership visible. The UART BSP owns the rings and
the handler; upper layers never access USART registers or NVIC directly.

The generic byte ring buffer lives in `common/`, so it can be reused by other
drivers without depending on UART or STM32.


## Dependency Enforcement

Run:

```bash
make check-layers
```

A successful build should not require weakening the checker. If a new include
is rejected, reconsider module placement before adding an exception.
