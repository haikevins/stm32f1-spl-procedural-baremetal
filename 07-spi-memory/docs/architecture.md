# Architecture — SPI Memory — W25Q64 NOR Flash

> **Scope:** Internal ownership, dependency direction, initialization, data flow, concurrency, timing, and failure propagation for `07-spi-memory`.

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

This example is not organized around “one source file per peripheral demo.” It keeps the repository's core rule: **Application expresses policy; lower layers own mechanisms and physical resources**. The concrete subject is SPI1 mode 0, software chip select, JEDEC identification, WEL/BUSY state, sector erase/page program/readback self-test.

The design should remain understandable in both directions:

- reading downward explains how a logical request reaches STM32 hardware;
- reading upward explains how low-level hardware state becomes a bounded, meaningful event/capability for Application.

## Layer and source map

```text
app/src/application.c
  -> memory_service + indication_service + time_service
services/src/memory_service.c
  -> w25q64 ECUAL
ecual/src/w25q64.c
  -> board_memory_bus
bsp/bluepill/src/board_memory_bus.c
  -> SPI1 PA5/PA6/PA7 + software CS PA4
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
board timebase + LED + SPI1 → Services → W25Q64 init/JEDEC validation → destructive Application self-test
```

The order matters because a module should never receive events or invoke a dependency before its state is valid. Peripheral flags are cleared and NVIC lines are configured only after associated storage/state is ready.

## Runtime data flow

```mermaid
flowchart TD
    ID["Read JEDEC ID with command 0x9F"] --> VALID{"Winbond manufacturer 0xEF and capacity 0x17?"}
    VALID -- "no" --> FAIL["Record error; steady LED"]
    VALID -- "yes" --> ERASE["WREN -> verify WEL -> 4 KiB erase 0x20"]
    ERASE --> READY1["Poll BUSY with bounded timeout"]
    READY1 --> PROGRAM["WREN -> page program 0x02, 32 bytes"]
    PROGRAM --> READY2["Poll BUSY with 50 ms bound"]
    READY2 --> READ["Read 0x03 into 32-byte buffer"]
    READ --> CMP{"Byte-for-byte equal?"}
    CMP -- "no" --> FAIL
    CMP -- "yes" --> PASS["Pass; toggle heartbeat every 500 ms"]
```

Data does not jump directly from an interrupt/peripheral into product policy. Every arrow has an owner and an API boundary. This lets the code document both **lifetime** and **authority** of the state being moved.

## Concurrency contract

SPI is polling/thread-mode only; SysTick continues to interrupt so timeouts and heartbeat time remain valid. There is no bus arbitration because this example has one SPI client.

### Key invariants

- CS is high when idle and one ECUAL command owns the complete CS-low transaction.
- Program/erase require WREN and verified WEL before the modifying command.
- Page program never crosses a 256-byte page boundary.
- Sector erase operates on a 4 KiB-aligned sector.
- Every busy/transfer wait is bounded.
- The configured last sector is disposable test data.

### Power-failure boundary

This example proves command sequencing and readback under normal execution. It does not implement an atomic storage scheme. Power loss during erase/program can leave the test sector partially modified; no metadata journal or redundant copy repairs that state.

The repository uses `volatile` for state that can change asynchronously, but `volatile` alone is not treated as a lock or a complete synchronization primitive. Where a compound take/clear or block copy must be atomic with respect to an ISR, the code uses a short PRIMASK critical section. Where SPSC ring publication depends on compiler ordering, it uses an explicit compiler barrier.

## Timing and memory reasoning

The project has no heap. Buffers, device state, counters, and Application state are statically or automatically allocated and are therefore visible in the link map.

Clock-dependent behavior is derived from CMSIS/SPL clock state wherever the example needs exact peripheral timing. The normal board configuration is 72 MHz HCLK, 72 MHz PCLK2, 36 MHz PCLK1, with APB1 timers receiving 72 MHz because their bus prescaler is not 1.

The most important timing-specific behavior for this example is described in its [README](../README.md); source constants under `config/` are authoritative.

## Failure and observability

Application exports JEDEC IDs, stage pass flags, error count, first mismatch index, and first/last readback bytes as GDB-friendly globals. The demo validates manufacturer `0xEF` and capacity `0x17`; it intentionally does not require one specific memory-type byte.

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

- [Winbond — W25Q64 product search/documentation](https://www.winbond.com/hq/search/?__locale=en&q=W25Q64JV)
- [STMicroelectronics — STM32F103 documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32f103/documentation.html)
- [STMicroelectronics — RM0008: STM32F101/102/103/105/107 reference manual](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [STMicroelectronics — PM0056: STM32F10xxx Cortex-M3 programming manual](https://www.st.com/resource/en/programming_manual/pm0056-stm32f10xxx20xxx21xxxl1xxxx-cortexm3-programming-manual-stmicroelectronics.pdf)
- [Arm — CMSIS Core documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [GNU Binutils — linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)
- [OpenOCD documentation](https://openocd.org/pages/documentation.html)
- [GDB — remote debugging](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Debugging.html)


---

[← Root](../../../README.md) · [↑ Examples](../../README.md) · [← Example README](../README.md) · [Architecture](architecture.md) · [Porting](porting_guide.md)
