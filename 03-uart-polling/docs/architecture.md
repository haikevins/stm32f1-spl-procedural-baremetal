# Architecture — UART Polling — Non-Blocking USART1

> **Scope:** Internal ownership, dependency direction, initialization, data flow, concurrency, timing, and failure propagation for `03-uart-polling`.

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

This example is not organized around “one source file per peripheral demo.” It keeps the repository's core rule: **Application expresses policy; lower layers own mechanisms and physical resources**. The concrete subject is USART1 115200 8N1 with `try_read`/`try_write`, no IRQ/DMA, one-byte pending echo state.

The design should remain understandable in both directions:

- reading downward explains how a logical request reaches STM32 hardware;
- reading upward explains how low-level hardware state becomes a bounded, meaningful event/capability for Application.

## Layer and source map

```text
app/src/application.c
  -> uart_service
services/src/uart_service.c
  -> board_uart try-read / try-write
bsp/bluepill/src/board_uart.c
  -> GPIOA + USART1 / SPL
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
board USART1 → UART Service → Application
```

The order matters because a module should never receive events or invoke a dependency before its state is valid. Peripheral flags are cleared and NVIC lines are configured only after associated storage/state is ready.

## Runtime data flow

```mermaid
flowchart TD
    INIT["Initialize USART1 115200 8N1"] --> BANNER["Startup message has bytes remaining?"]
    BANNER -- "yes" --> TXREADY{"TXE set?"}
    TXREADY -- "yes" --> SEND["Write one banner byte"]
    TXREADY -- "no" --> RETURN["Return to super-loop"]
    SEND --> RETURN
    BANNER -- "no" --> PENDING{"Echo byte pending?"}
    PENDING -- "no" --> RXREADY{"RXNE set?"}
    RXREADY -- "yes" --> STORE["Read byte and mark pending"]
    RXREADY -- "no" --> RETURN
    STORE --> PENDING
    PENDING -- "yes" --> ECHOTX{"TXE set?"}
    ECHOTX -- "yes" --> ECHO["Write byte and clear pending"]
    ECHOTX -- "no" --> RETURN
    ECHO --> RETURN
```

Data does not jump directly from an interrupt/peripheral into product policy. Every arrow has an owner and an API boundary. This lets the code document both **lifetime** and **authority** of the state being moved.

## Concurrency contract

This example is intentionally thread-only for USART. The super-loop is the sole reader/writer; there is no shared UART state with an ISR. That makes it a baseline for understanding why the ownership model changes in Example 04.

### Key invariants

- UART has exactly one software execution context: thread mode.
- No BSP function waits for TXE/RXNE; each reports readiness immediately.
- When `s_echo_pending` is true, Application must transmit that byte before consuming another RX byte.
- Startup-message progress advances only after a successful TX write.

### Throughput consequence

Because there is no RX queue, correctness under sustained input depends on loop service latency relative to character arrival. At 115200 8N1, a full 10-bit frame is roughly 86.8 µs; a loop that is delayed far longer by other work can allow hardware overrun. This example intentionally exposes that limitation.

The repository uses `volatile` for state that can change asynchronously, but `volatile` alone is not treated as a lock or a complete synchronization primitive. Where a compound take/clear or block copy must be atomic with respect to an ISR, the code uses a short PRIMASK critical section. Where SPSC ring publication depends on compiler ordering, it uses an explicit compiler barrier.

## Timing and memory reasoning

The project has no heap. Buffers, device state, counters, and Application state are statically or automatically allocated and are therefore visible in the link map.

Clock-dependent behavior is derived from CMSIS/SPL clock state wherever the example needs exact peripheral timing. The normal board configuration is 72 MHz HCLK, 72 MHz PCLK2, 36 MHz PCLK1, with APB1 timers receiving 72 MHz because their bus prescaler is not 1.

The most important timing-specific behavior for this example is described in its [README](../README.md); source constants under `config/` are authoritative.

## Failure and observability

The current API does not surface framing/parity/noise/overrun diagnostics to Application. It demonstrates readiness polling, not a production serial error-recovery channel.

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
