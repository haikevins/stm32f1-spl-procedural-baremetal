# Architecture - 08 - ADC + DMA

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


Data path:

```text
TIM3 update @ 1 kHz
        |
        v
ADC1 conversion PA0
        |
        v
DMA1 Channel 1 circular buffer [64]
        |
        +--> HT: samples 0..31
        |
        +--> TC: samples 32..63
        |
DMA1_Channel1_IRQHandler
        |
        +--> copy completed half into stable 32-sample block
        +--> mark block ready
        +--> count overrun/error
        |
thread mode
        |
adc_service_process()
        |
        +--> min
        +--> max
        +--> rounded average
        +--> estimated millivolts
        |
Application
        |
        +--> diagnostics
        +--> PC13 hysteresis
```

The ISR intentionally does not calculate statistics or apply LED policy.


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


`board_adc_dma_init()` performs:

```text
enable DMA1 / GPIOA / ADC1 / TIM3 clocks
    |
configure ADC clock PCLK2/6
    |
PA0 analog input
    |
DMA1 CH1:
    peripheral = ADC1->DR
    memory     = 64-sample buffer
    16-bit peripheral + memory width
    memory increment
    circular mode
    high priority
    HT + TC + TE interrupts
    |
NVIC DMA1_Channel1
    |
ADC1:
    independent
    single regular channel
    external TIM3 TRGO
    right aligned
    55.5-cycle sample time
    |
TIM3:
    computed PSC/ARR
    TRGO = update
    |
ADC calibration
    |
enable external trigger
    |
start TIM3
```

No ADC interrupt or TIM3 interrupt is required.


## Low-Level Ownership


DMA uses circular mode and half-transfer/full-transfer interrupts.

The BSP copies the completed half into a separate stable block before
publishing it. If a new half arrives while the previous block is still pending,
the overrun counter increments and the newest completed half replaces the
published block.

Thread mode copies the published block under a short PRIMASK critical section.

Millivolts are estimated from:

```text
mV = average_raw * 3300 / 4095
```

The result is only as accurate as the `3300 mV` VDDA assumption.


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


This is the clearest producer/consumer example in the repository. Hardware and
DMA continuously produce samples, the BSP converts interrupt completion into a
stable block, the Service converts samples into a measurement, and Application
owns only threshold policy.


## Dependency Enforcement

Run:

```bash
make check-layers
```

A successful build should not require weakening the checker. If a new include
is rejected, reconsider module placement before adding an exception.
