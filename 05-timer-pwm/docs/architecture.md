# Architecture - 05 - Timer PWM

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


TIM2 generates the waveform continuously in hardware.

Every 10 ms the Application changes its desired duty by 10 permille:

```text
0 -> 10 -> ... -> 1000 -> 990 -> ... -> 0 -> repeat
```

`pwm_service_set_duty_permille()` passes the logical duty to the BSP.

The BSP calculates compare counts with rounding:

```text
compare = duty_permille * period_counts / 1000
```

No TIM2 interrupt is needed.


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


`board_pwm_init()` determines the TIM2 input clock through
`RCC_GetClocksFreq()`. Because TIM2 is on APB1, it doubles PCLK1 when the APB1
prescaler is not 1.

The BSP then calculates:

```text
prescaler divider = timer_clock / 1 MHz
period counts     = 1 MHz / 1 kHz = 1000
```

TIM2 CH1 is configured for PWM mode 1 with output and preload enabled.

The board then starts the 1 ms SysTick timebase used only to decide when the
Application changes duty.


## Low-Level Ownership


Important SPL concepts:

- `TIM_TimeBaseInit()` sets PSC/ARR;
- `TIM_OC1Init()` selects PWM1;
- `TIM_OC1PreloadConfig()` enables CCR preload;
- `TIM_ARRPreloadConfig()` enables ARR preload;
- `TIM_SetCompare1()` updates duty;
- `TIM_Cmd()` starts TIM2.

SysTick is a separate scheduler timebase and does not generate the PWM waveform.


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


This example separates scheduling from signal generation. SysTick tells the
Application when to change state; TIM2 owns the precise PWM edges. That pattern
scales better than software-toggling a GPIO at PWM frequency.


## Dependency Enforcement

Run:

```bash
make check-layers
```

A successful build should not require weakening the checker. If a new include
is rejected, reconsider module placement before adding an exception.
