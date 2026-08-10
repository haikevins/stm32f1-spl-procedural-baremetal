# Architecture - 03 - UART Polling

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


The Application first sends:

```text
STM32F103 UART polling ready
Type characters to echo.
```

The message is not transmitted by a blocking string function. Each
`application_process()` call attempts one byte when `TXE` is ready.

After the greeting, the state machine keeps at most one pending echo byte:

```text
RXNE ready?
    |
    +--> read byte
          |
          v
      pending echo
          |
TXE ready?
    |
    +--> write byte
```

If hardware is not ready, the function simply returns to the super-loop.


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


```text
board_init()
    |
    +--> SystemCoreClockUpdate()
    +--> board_uart_init()
            |
            +--> GPIOA + USART1 clocks
            +--> PA9 AF push-pull
            +--> PA10 floating input
            +--> USART 115200 8N1
            +--> RX + TX enable

system_init()
    |
    +--> uart_service_init()
    +--> application_init()
```

`uart_service_init()` has no hardware work because the BSP already owns
peripheral initialization.


## Low-Level Ownership


`board_uart_try_read_byte()` checks `USART_FLAG_RXNE`.

`board_uart_try_write_byte()` checks `USART_FLAG_TXE`.

There is no USART IRQ, no DMA, and no software queue. This makes the example a
useful baseline for comparison with Example 04.


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


The key lesson is API shape. The upper layers already use `try_read` and
`try_write`, which means Example 04 can change the transport implementation to
interrupt-driven rings while preserving a similar non-blocking Application
model.


## Dependency Enforcement

Run:

```bash
make check-layers
```

A successful build should not require weakening the checker. If a new include
is rejected, reconsider module placement before adding an exception.
