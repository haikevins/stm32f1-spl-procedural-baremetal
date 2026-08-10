# Architecture — 01-blink-led

## 1. Dependency Graph

```text
Application
    |
    +--> Time Service
    |       |
    |       v
    |   Board Timebase
    |       |
    |       v
    |    CMSIS SysTick
    |
    +--> Indication Service
            |
            v
         Board LED
            |
            v
         GPIO/RCC SPL
```

## 2. Layer Responsibilities

**Application**

Owns the 500 ms blink policy.

**Time Service**

Exposes millisecond timing.

**Indication Service**

Exposes logical indication operations.

**Board Timebase**

Owns SysTick configuration and `SysTick_Handler()`.

**Board LED**

Owns PC13, GPIOC clock, output mode, and active-low translation.

**System**

Owns initialization order.

## 3. Initialization Dependency

The board must be initialized before Services.

```text
System clock information
    |
Board LED + Board Timebase
    |
Time/Indication Services
    |
Application
```

The Application timestamp is initialized only after the timebase exists.

## 4. Runtime Data Flow

### Time Path

```text
SysTick interrupt
    |
s_time_ms++
    |
Time Service
    |
Application periodic check
```

### LED Path

```text
Application
    |
Indication Service
    |
Board LED
    |
GPIO SPL
    |
PC13
```

## 5. Peripheral Ownership

Only the BSP owns physical peripheral configuration.

Application does not know:

- GPIOC;
- PC13;
- RCC clock bit;
- output speed;
- active-low polarity.

This is the main separation demonstrated by Example 01.

## 6. ISR Ownership

`SysTick_Handler()` is implemented by the Board Timebase.

The handler is intentionally minimal:

```text
interrupt -> increment counter -> return
```

No upward callback is used.

## 7. Concurrency

`volatile uint32_t s_time_ms` is written by ISR context and read by thread mode.

A 32-bit aligned read/write is naturally atomic on Cortex-M3 for this use case.

Timestamp subtraction uses unsigned wraparound-safe arithmetic.

## 8. Clock Dependency

The SPL/CMSIS system clock setup is outside Application.

The Board Timebase derives SysTick reload from `SystemCoreClock`.

When the system clock changes, the timebase remains correct as long as
`SystemCoreClockUpdate()` reflects the new clock before SysTick configuration.

## 9. Why Application Does Not Use a Busy Delay

A busy delay would prevent other work from running during the entire delay.

The timestamp pattern instead allows:

```text
check time -> no work due -> return
```

This is the foundation for every later super-loop example.

## 10. Current Event-Service Scope

This example does not need a general event queue.

The Time Service and Indication Service are deliberately small because the
purpose is to show the first clean dependency boundary.

Add a generic event system only when the product requirements justify it.

## 11. Failure Path

If `board_timebase_init()` fails:

```text
board_init() -> false
system_init() -> false
main() -> system_panic()
```

The failure does not continue into Application with an invalid timebase.

## 12. Extension Boundaries

Good extensions:

- new timing policy in Application;
- new logical indication in Service;
- different LED pin in BSP;
- different timebase peripheral in BSP.

Avoid adding GPIO/SysTick calls directly to Application.
