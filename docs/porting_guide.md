# Porting Guide — Project Template

> **Scope:** A layered porting method for moving the template between boards, STM32F1 densities/parts, Cortex-M MCU families, or different CPU architectures without confusing board changes with Application changes.

[Main](https://github.com/haikevins/stm32f1-spl-procedural-baremetal) · [← Template README](../README.md) · [Architecture](architecture.md) · [Adding a module](adding_a_module.md)

## Table of contents

- [Porting by change scope](#porting-by-change-scope)
- [Same STM32F103C8T6, new board](#same-stm32f103c8t6-new-board)
- [Different STM32F1 part](#different-stm32f1-part)
- [Different Cortex-M MCU family](#different-cortex-m-mcu-family)
- [Different CPU architecture](#different-cpu-architecture)
- [Clock, linker, and startup checklist](#clock-linker-and-startup-checklist)
- [Peripheral and interrupt checklist](#peripheral-and-interrupt-checklist)
- [Validation order](#validation-order)
- [References](#references)

## Porting by change scope

Use the smallest replacement boundary that matches the physical change:

| Change | Usually preserve | Rework/review first |
|---|---|---|
| new project, same Blue Pill | startup/linker/vendor/tooling | Application, Services, BSP resources, selected SPL modules |
| different board, same C8 MCU | Application/Services/ECUAL/Common/startup/linker | BSP pins/electrical assumptions, config, OpenOCD/reset wiring |
| different STM32F103 density | higher layers | linker memory, device density define, vector/startup compatibility |
| different STM32F1 part | much of high-level policy | peripheral availability/remap, IRQ/DMA mapping, clock, linker/startup |
| newer STM32 Cortex-M family | Application/Services/ECUAL concepts | vendor layer, BSP, startup, linker, clocks, debug target |
| different CPU architecture | high-level conceptual separation | startup ABI, linker, critical sections, interrupt model, compiler/tooling |

## Same STM32F103C8T6, new board

Review every physical resource:

```text
logical name -> GPIO port/pin -> mode/polarity -> peripheral AF/remap -> connector/electrical circuit
```

Do not encode the new pin in Application. If an SSD1306 or W25Q64 remains the same IC, retain ECUAL protocol code and adapt its board-bus boundary.

OpenOCD currently assumes ST-Link/SWD and `reset_config none`; if the new board exposes NRST, a hardware-reset configuration can be adopted after validation.

## Different STM32F1 part

Review:

- Flash and SRAM origin/length;
- medium/high/value-line density preprocessor define;
- vector table contents and exact IRQ names;
- alternate-function/remap differences;
- available timer/ADC/SPI/I2C/USART instances;
- DMA channel mapping;
- clock-tree limits;
- SPL support for that part.

Never use the C8 linker map merely because another package is also “STM32F103.”

## Different Cortex-M MCU family

The Application/Service/BSP/ECUAL decomposition can survive, but the checked-in STM32F1 SPL cannot. Replace the vendor/device layer and reimplement BSP against the new family API/register model. Also replace startup/device header, linker memory, clock initialization, and OpenOCD target.

Review architecture-level assumptions such as NVIC priority bits, available DMA architecture, cache/coherency behavior, and whether compiler barriers/PRIMASK sections still satisfy the new concurrency model.

## Different CPU architecture

At that point even CMSIS/Cortex-M conventions are no longer portable. Port:

- reset/exception entry ABI;
- vector/interrupt table model;
- stack initialization;
- linker section/memory conventions;
- critical-section primitive;
- memory-order/barrier semantics;
- compiler CPU/ISA flags;
- debug transport/server.

The conceptual layers can remain useful, but do not disguise architecture-specific code as “common.”

## Clock, linker, and startup checklist

### Clock

- oscillator frequency/source is correct;
- vendor startup reaches the intended SYSCLK;
- `SystemCoreClock` matches reality;
- AHB/APB clocks are known;
- APB timer doubling is handled where applicable;
- peripheral maximum clocks remain legal.

### Linker

- Flash/RAM origin and length match the exact part;
- `.isr_vector` is retained at reset vector address;
- `.data` has correct Flash load/RAM execution addresses;
- `.bss` is zeroed range;
- `_estack` is valid/aligned;
- static data + stack headroom fit SRAM.

### Startup

- reset entry matches ISA;
- `.data/.bss` loops match linker symbols;
- exact vector names match peripheral handlers;
- `SystemInit()`/equivalent is called at the intended point.

## Peripheral and interrupt checklist

For each peripheral verify as one unit:

1. bus clock enable/reset;
2. physical pins and AF mode;
3. peripheral clock/prescaler;
4. init fields and legal ranges;
5. stale-flag clearing;
6. IRQ/DMA request mapping and priority;
7. handler acknowledgement sequence;
8. shared-memory ownership/capacity;
9. timeout/drop/retry policy;
10. higher-layer API semantics.

## Validation order

```text
Reset / vector
    ↓
.data / .bss / stack
    ↓
clock tree
    ↓
GPIO safe states
    ↓
basic peripheral operation
    ↓
IRQ / DMA concurrency
    ↓
Service contract
    ↓
Application behavior
    ↓
stress / error / power / reset limits
```

Keep bring-up evidence close to the lowest boundary. A wrong UART baud is more efficiently diagnosed from clocks/BRR and a logic analyzer than from Application code.

## References

- [STMicroelectronics — STM32F103 documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32f103/documentation.html)
- [STMicroelectronics — RM0008: STM32F101/102/103/105/107 reference manual](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [STMicroelectronics — PM0056: STM32F10xxx Cortex-M3 programming manual](https://www.st.com/resource/en/programming_manual/pm0056-stm32f10xxx20xxx21xxxl1xxxx-cortexm3-programming-manual-stmicroelectronics.pdf)
- [Arm — CMSIS Core documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [GNU Binutils — linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)
- [OpenOCD documentation](https://openocd.org/pages/documentation.html)
- [GDB — remote debugging](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Debugging.html)
