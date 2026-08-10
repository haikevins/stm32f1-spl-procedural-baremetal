# Architecture - 01 - Blink LED

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


Application stores the last toggle timestamp. Every call to
`application_process()` asks `time_service_periodic_due()` whether 500 ms has
elapsed.

```text
super-loop
    |
    +--> application_process()
            |
            +--> time_service_periodic_due()
                    |
              false +--> return immediately
                    |
               true +--> indication_service_toggle()
```

No delay loop is used to create the blink period.


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
SystemInit()
    |
main()
    |
system_init()
    |
    +--> board_init()
    |      +--> SystemCoreClockUpdate()
    |      +--> board_led_init()
    |      +--> board_timebase_init()
    |
    +--> time_service_init()
    +--> indication_service_init()
    +--> application_init()
```

`board_timebase_init()` configures SysTick from `SystemCoreClock /
BOARD_TIMEBASE_HZ`. `SysTick_Handler()` increments a millisecond counter.


## Low-Level Ownership


SPL usage is intentionally small:

- `RCC_APB2PeriphClockCmd()` enables GPIOC.
- `GPIO_Init()` configures PC13 as 2 MHz push-pull output.
- `SysTick_Config()` creates the 1 kHz timebase.
- `NVIC_SetPriority()` places SysTick at the lowest implemented priority.

The only active interrupt is SysTick.


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


The main architectural lesson is that the Application does not know that the
indicator is PC13 or active-low. It only asks for the logical status indicator
to toggle. This makes a later change to a different LED or output device a BSP
change rather than an Application rewrite.


## Dependency Enforcement

Run:

```bash
make check-layers
```

A successful build should not require weakening the checker. If a new include
is rejected, reconsider module placement before adding an exception.
