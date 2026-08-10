# Architecture — 02-gpio-input-interrupt

## 1. Layer Diagram

```text
Application
    |
    +--> Button Service
    |       |
    |       +--> Board Button
    |       |       |
    |       |       +--> GPIO/AFIO/EXTI SPL
    |       |
    |       +--> Time Service
    |               |
    |               v
    |          Board Timebase
    |
    +--> Indication Service
            |
            v
         Board LED
```

## 2. Ownership

| Concern | Owner |
|---|---|
| PA0 electrical mode | Board Button |
| EXTI0 mapping | Board Button |
| EXTI0 IRQ | Board Button |
| raw press-edge flag | Board Button |
| 30 ms debounce | Button Service |
| debounced press event | Button Service |
| toggle policy | Application |
| PC13 polarity | Board LED |

## 3. Why Debounce Is Not in the ISR

Mechanical bounce can last milliseconds.

Waiting inside an ISR would:

- block lower-priority interrupts;
- increase interrupt latency;
- couple hardware capture with policy;
- make timing harder to reason about.

The ISR records the edge only. Thread mode owns elapsed-time validation.

## 4. EXTI Event Lifecycle

```text
physical falling edge
    |
EXTI0 pending
    |
EXTI0_IRQHandler
    |
s_press_edge_pending = true
    |
thread takes raw edge
    |
30 ms debounce window
    |
sample PA0
    |
pressed event pending
    |
Application takes event
    |
toggle indication
```

## 5. Concurrency Model

One boolean is shared between ISR and thread mode.

The BSP protects the take-and-clear operation using saved PRIMASK state.

This is enough because:

- producer is one ISR;
- consumer is one thread;
- only one pending edge needs to be retained for the debounce strategy.

If every edge count mattered, a counter/queue would be required.

## 6. Initialization Order

The correct order is:

```text
LED + timebase + button hardware
    |
Services
    |
Application
```

The Button Service requires a working timebase for debounce.

## 7. Layer Boundaries

Application may include:

```text
button_service.h
indication_service.h
```

It must not include:

```text
board_button.h
stm32f10x_exti.h
stm32f10x_gpio.h
```

## 8. Generic EXTI Abstraction

This SPL example keeps EXTI configuration directly in the Board Button module
instead of introducing a separate generic MCAL layer.

That is acceptable because SPL itself acts as the low-level peripheral layer.

If many board inputs require EXTI, a reusable lower-level EXTI wrapper may be
introduced, but it should remain below Services.

## 9. Failure Propagation

Board initialization functions in this example are mostly void except for the
timebase. A SysTick configuration failure causes:

```text
board_init() -> false
system_init() -> false
system_panic()
```

Runtime bounce/noise is handled as a normal event-filtering concern rather than
a fatal error.

## 10. Extension Strategy

For richer button behavior:

```text
Board Button
    |
raw edge/level
    |
Button Service
    |
press/release/long/double events
    |
Application
```

Keep the physical pin and EXTI details at the bottom even as the Service grows.
