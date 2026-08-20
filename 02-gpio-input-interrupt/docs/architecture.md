# Architecture — GPIO Input Interrupt — EXTI + Debounce

> **Scope:** Internal ownership, dependency direction, initialization, data flow, concurrency, timing, and failure propagation for `02-gpio-input-interrupt`.

[← Root](../../../README.md) · [↑ Examples](../../README.md) · [← Example README](../README.md) · [Architecture](architecture.md) · [Porting](porting_guide.md)

## Table of contents

- [Architectural objective](#architectural-objective)
- [Layer and source map](#layer-and-source-map)
- [Composition and initialization](#composition-and-initialization)
- [Runtime data flow](#runtime-data-flow)
- [Concurrency contract](#concurrency-contract)
- [Timing and memory reasoning](#timing-and-memory-reasoning)
- [Failure and observability](#failure-and-observability)
- [Extension boundaries](#extension-boundaries)
- [References](#references)

## Architectural objective

This example is not organized around “one source file per peripheral demo.” It keeps the repository's core rule: **Application expresses policy; lower layers own mechanisms and physical resources**. The concrete subject is active-low button, AFIO/EXTI mapping, minimal ISR publication, deferred 30 ms debounce.

The design should remain understandable in both directions:

- reading downward explains how a logical request reaches STM32 hardware;
- reading upward explains how low-level hardware state becomes a bounded, meaningful event/capability for Application.

## Layer and source map

```text
app/src/application.c
  -> button_service + indication_service
services/src/button_service.c
  -> board_button + time_service
bsp/bluepill/src/board_button.c
  -> GPIOA + AFIO + EXTI0 + NVIC
bsp/bluepill/src/board_timebase.c
  -> SysTick
```

```mermaid
flowchart TD
    SYS["system/system_init.c: composition root"] --> APP["app: policy"]
    SYS --> SVC["services: logical capability"]
    SYS --> BSP["bsp/bluepill: resource ownership"]
    APP --> SVC
    SVC --> BSP
    SVC --> ECUAL["ecual: off-chip protocol when used"]
    ECUAL --> BSP
    BSP --> SPL["SPL/CMSIS"]
    SPL --> HW["STM32 / external hardware"]
```

The layer checker is part of the architecture contract. A lower-layer implementation can change without authorizing Application to bypass its public Service interface.

## Composition and initialization

The reset path is common to the repository. After `.data/.bss` initialization and `SystemInit()`, `main()` delegates composition to `system_init()`.

For this example the important dependency order is:

```text
board LED/button/timebase → Time Service → Indication Service → Button Service → Application
```

The order matters because a module should never receive events or invoke a dependency before its state is valid. Peripheral flags are cleared and NVIC lines are configured only after associated storage/state is ready.

## Runtime data flow

```mermaid
sequenceDiagram
    participant BTN as PA0 button
    participant ISR as EXTI0_IRQHandler
    participant FLAG as Pending-edge flag
    participant SVC as button_service_process
    participant APP as application_process

    BTN->>ISR: falling edge
    ISR->>FLAG: pending = true
    ISR->>ISR: clear EXTI0 pending bit
    SVC->>FLAG: atomic take-and-clear
    SVC->>SVC: start/restart 30 ms debounce
    SVC->>BTN: sample pin after window
    alt still LOW
        SVC->>APP: publish pressed event
        APP->>APP: toggle indication
    else HIGH again
        SVC->>SVC: reject bounce/transient
    end
```

Data does not jump directly from an interrupt/peripheral into product policy. Every arrow has an owner and an API boundary. This lets the code document both **lifetime** and **authority** of the state being moved.

## Concurrency contract

SysTick updates time; EXTI0 publishes the raw candidate. PRIMASK protects take-and-clear. Debounce state and logical pressed-event state are owned by thread mode.

### Key invariants

- EXTI0 ISR publishes only a raw candidate; it never turns a candidate into a user-level press.
- `take_press_edge` read/clear is atomic with respect to EXTI because PRIMASK surrounds it.
- A logical press is published only after the 30 ms window and a fresh physical-pin read.
- Button polarity/mapping remain below the Service.

### Race reasoning

If a new edge occurs before thread mode takes the boolean, it coalesces. If it occurs while thread mode holds PRIMASK for read/clear, the exception is delayed until interrupts are restored, after which it can set the flag again. This avoids a lost edge specifically at the read/clear boundary, while still intentionally allowing coalescing outside it.

The repository uses `volatile` for state that can change asynchronously, but `volatile` alone is not treated as a lock or a complete synchronization primitive. Where a compound take/clear or block copy must be atomic with respect to an ISR, the code uses a short PRIMASK critical section. Where SPSC ring publication depends on compiler ordering, it uses an explicit compiler barrier.

## Timing and memory reasoning

The project has no heap. Buffers, device state, counters, and Application state are statically or automatically allocated and are therefore visible in the link map.

Clock-dependent behavior is derived from CMSIS/SPL clock state wherever the example needs exact peripheral timing. The normal board configuration is 72 MHz HCLK, 72 MHz PCLK2, 36 MHz PCLK1, with APB1 timers receiving 72 MHz because their bus prescaler is not 1.

The most important timing-specific behavior for this example is described in its [README](../README.md); source constants under `config/` are authoritative.

## Failure and observability

There is no runtime retry mechanism because GPIO/EXTI have no transactional device fault here. A wrong pin source, polarity, pending-bit sequence, or timebase manifests as missing/repeated press events and should be diagnosed at the BSP boundary.

Initialization failure propagates toward `system_init()` instead of being silently ignored. Runtime diagnostics are deliberately low-overhead: counters, state flags, and GDB-visible globals are preferred to adding a logging subsystem that would change the example's peripheral/concurrency profile.

## Extension boundaries

When extending this example:

1. keep board pin/peripheral mapping in BSP/configuration;
2. keep external-device command semantics in ECUAL when an off-chip device is involved;
3. expose a Service capability rather than an SPL type to Application;
4. keep ISR work bounded and define the publication/ownership rule before writing the handler;
5. update `config/modules.mk` only with the SPL sources actually required;
6. run `make check-layers` before accepting the change;
7. document any new buffer capacity, timeout, drop policy, interrupt priority, or destructive operation.

## References

- [STMicroelectronics — STM32F103 documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32f103/documentation.html)
- [STMicroelectronics — RM0008: STM32F101/102/103/105/107 reference manual](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [STMicroelectronics — PM0056: STM32F10xxx Cortex-M3 programming manual](https://www.st.com/resource/en/programming_manual/pm0056-stm32f10xxx20xxx21xxxl1xxxx-cortexm3-programming-manual-stmicroelectronics.pdf)
- [Arm — CMSIS Core documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [GNU Binutils — linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)
- [OpenOCD documentation](https://openocd.org/pages/documentation.html)
- [GDB — remote debugging](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Debugging.html)


---

[← Root](../../../README.md) · [↑ Examples](../../README.md) · [← Example README](../README.md) · [Architecture](architecture.md) · [Porting](porting_guide.md)
