# Architecture - 02 - GPIO Input Interrupt

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


The ISR does not debounce:

```text
PA0 falling edge
    |
EXTI0_IRQHandler
    |
    +--> set s_press_edge_pending
    +--> clear EXTI pending bit
    |
return
```

Thread mode performs debounce:

```text
button_service_process()
    |
    +--> take low-level edge?
    |       |
    |       +--> record start time, enable debounce
    |
    +--> 30 ms elapsed?
            |
            +--> no: return
            |
            +--> yes: sample PA0
                       |
                       +--> still pressed -> publish pressed event

application_process()
    |
    +--> take pressed event?
            |
            +--> toggle logical indicator
```

Every new falling edge restarts the debounce window, absorbing mechanical
bounce without blocking the ISR or super-loop.


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
    +--> board_led_init()
    +--> board_timebase_init()
    +--> board_button_init()
            |
            +--> GPIOA + AFIO clocks
            +--> PA0 input pull-up
            +--> GPIO_EXTILineConfig()
            +--> EXTI falling-edge config
            +--> clear pending EXTI0
            +--> NVIC priority + enable

system_init()
    |
    +--> time_service_init()
    +--> indication_service_init()
    +--> button_service_init()
    +--> application_init()
```

`button_service_init()` discards any edge captured during startup.


## Low-Level Ownership


The board layer uses SPL EXTI/GPIO/RCC APIs and CMSIS NVIC primitives.

The ISR-owned/shared item is the `volatile bool s_press_edge_pending`. Thread
mode clears it using a short PRIMASK-protected critical section so an
interrupt cannot be lost between read and clear.


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


This example demonstrates the repository's interrupt ownership rule: the BSP
owns EXTI0 because it owns the physical button. The Service owns debounce
policy. The Application owns the decision that a debounced press toggles the
status indication.


## Dependency Enforcement

Run:

```bash
make check-layers
```

A successful build should not require weakening the checker. If a new include
is rejected, reconsider module placement before adding an exception.
