# STM32F1 SPL Procedural Bare-Metal — Examples

> **Scope:** Eight independently buildable projects that introduce STM32F103 peripherals and increasingly sophisticated data-handoff patterns while preserving the same layered firmware architecture.

[← Main README](https://github.com/haikevins/stm32f1-spl-procedural-baremetal) · [Template branch](https://github.com/haikevins/stm32f1-spl-procedural-baremetal/tree/template)

## Table of contents

- [Learning model](#learning-model)
- [Roadmap](#roadmap)
- [Cross-example comparison](#cross-example-comparison)
- [Wiring summary](#wiring-summary)
- [Common architecture](#common-architecture)
- [Concurrency progression](#concurrency-progression)
- [Build and debug workflow](#build-and-debug-workflow)
- [How to read each example](#how-to-read-each-example)
- [References](#references)

## Learning model

The examples are intentionally progressive. The peripheral changes, but the architectural vocabulary does not:

```text
01  GPIO + SysTick
        ↓
02  EXTI handoff + debounce
        ↓
03  UART polling
        ↓
04  UART IRQ + SPSC rings
        ↓
05  hardware PWM
        ↓
06  I2C + SSD1306 ECUAL
        ↓
07  SPI NOR protocol
        ↓
08  ADC + DMA pipeline
```

Each stage asks two questions at once:

1. **How does this STM32 peripheral mechanism work?**
2. **Which layer should own each part of that mechanism?**

This prevents the examples from degenerating into unrelated `main.c` snippets.

## Roadmap

### 01 — GPIO output and timebase

[Open Example 01](01-blink-led/README.md)

PC13 active-low LED + 1 kHz SysTick. Introduces board polarity, Service abstraction, wrap-safe millisecond time, and a cooperative periodic action.

### 02 — EXTI input and debounce

[Open Example 02](02-gpio-input-interrupt/README.md)

PA0 active-low button with internal pull-up. EXTI0 publishes a minimal event; a Service performs 30 ms debounce later in thread mode. This is the first explicit interrupt-to-super-loop handoff.

### 03 — Non-blocking UART polling

[Open Example 03](03-uart-polling/README.md)

USART1 at 115200 8N1. `try_read`/`try_write` expose hardware readiness without blocking; the application stores at most one pending echo byte.

### 04 — UART interrupts and ring buffers

[Open Example 04](04-uart-interrupt-ring-buffer/README.md)

The same USART becomes asynchronous. RX/TX 128-byte arrays are used as SPSC rings with 127-byte usable capacity. The USART ISR and main loop have explicit producer/consumer ownership, and TXE interrupts are enabled only while data needs draining.

### 05 — Timer-generated PWM

[Open Example 05](05-timer-pwm/README.md)

TIM2_CH1 on PA0 runs a 1 kHz hardware waveform from a 1 MHz timer tick. The application updates only CCR-derived duty policy every 10 ms; waveform edges are entirely hardware-owned.

### 06 — I2C and an external display device

[Open Example 06](06-i2c-display/README.md)

I2C1 at 400 kHz drives an SSD1306 through an ECUAL driver and 1024-byte framebuffer. The example separates bus electrical/timing ownership from SSD1306 command semantics and display policy.

### 07 — SPI NOR memory

[Open Example 07](07-spi-memory/README.md)

SPI1 mode 0 drives a W25Q64. The ECUAL layer models JEDEC ID, WEL/BUSY, sector erase, page programming, and readback. A destructive reset-time self-test validates one 32-byte pattern in the last 4 KiB sector.

### 08 — Timer-triggered ADC + circular DMA

[Open Example 08](08-adc-dma/README.md)

TIM3 TRGO triggers ADC1 at 1 ksample/s; DMA1 Channel 1 fills a 64-sample circular buffer. Half/full interrupts copy 32-sample blocks into a one-slot staging buffer, then thread mode computes min/max/average/millivolts and drives LED hysteresis.

## Cross-example comparison

| Example | Peripheral owner | Interrupt source | Handoff / processing model | Main timing characteristic |
|---|---|---|---|---|
| 01 | BSP timebase + LED | SysTick | volatile millisecond counter | 500 ms toggle |
| 02 | BSP button/timebase | EXTI0, SysTick | edge flag + PRIMASK take + debounce | 30 ms qualification |
| 03 | BSP UART | none | direct non-blocking polling | loop-frequency dependent |
| 04 | BSP UART | USART1 | SPSC RX/TX rings | 32 operations max/application call |
| 05 | BSP PWM/timebase | SysTick only | hardware PWM; thread changes duty | 1 kHz carrier, 10 ms duty step |
| 06 | BSP I2C/timebase | SysTick | synchronous bounded I2C + framebuffer | 400 kHz bus, 100 ms UI step |
| 07 | BSP SPI/timebase | SysTick | synchronous bounded NOR transactions | erase may wait up to 2 s |
| 08 | BSP ADC/DMA | DMA1 CH1 | ISR block copy + one-slot latest block | 1 ksample/s, 32 ms block |

## Wiring summary

### SWD — all examples

```text
ST-Link            Blue Pill
--------------------------------
SWDIO      ------> PA13 / SWDIO
SWCLK      ------> PA14 / SWCLK
GND        ------- GND
3.3 V ref  ------- 3.3 V
```

The OpenOCD configuration does not require NRST.

### Example-specific I/O

| Example | Wiring |
|---|---|
| 01 | onboard PC13 LED only |
| 02 | PA0 → push button → GND; internal pull-up enabled |
| 03/04 | PA9 TX → USB-UART RX, PA10 RX ← USB-UART TX, common GND, 3.3 V logic |
| 05 | PA0/TIM2_CH1 → 330 Ω → LED → GND |
| 06 | PB6 → SSD1306 SCL, PB7 ↔ SDA, 3.3 V/GND; bus requires pull-ups |
| 07 | PA4 CS, PA5 SCK, PA6 MISO, PA7 MOSI to W25Q64; 3.3 V/GND |
| 08 | analog source/potentiometer wiper → PA0/ADC1_IN0; source must remain within MCU analog limits |

## Common architecture

```text
Application policy
        ↓
Services
   ├──→ BSP ─────→ SPL / CMSIS ─────→ STM32 hardware
   └──→ ECUAL ───→ BSP

System : composition root
Common : portable shared types/utilities
```

`system/system_init.c` is the composition root in every project. Peripheral-specific initialization lives below it, and Application does not include raw board/vendor headers. `tools/scripts/check_layers.py` enforces this include-dependency contract.

## Concurrency progression

The sequence is deliberately educational:

```text
01: periodic counter
      ↓
02: ISR flag -> thread-mode qualification
      ↓
03: thread-only hardware polling
      ↓
04: ISR/thread SPSC byte streams
      ↓
05: hardware waveform engine + slow software control
      ↓
06/07: bounded synchronous device transactions
      ↓
08: hardware trigger -> ADC -> DMA -> ISR block publication -> thread processing
```

A recurring invariant is that higher-level policy is never executed inside the lowest-level interrupt handler.

## Build and debug workflow

```bash
make check-layers
make clean
make
```

The build creates `build/firmware.elf`, `.hex`, `.bin`, `.lst`, dependency files, and a linker map. Useful targets are:

```bash
make size
make tree
make flash
make erase
make debug-server
make debug
```

`make all` runs the architectural layer checker before compilation. The Makefile targets Cortex-M3/Thumb, compiles C11 with `-Og -g3`, places each function/data object in its own section, links with the project linker script, enables linker garbage collection, and deliberately uses `-nostartfiles -nostdlib`. Only compiler runtime support (`-lgcc`) is linked explicitly.

The repository uses ST-Link/SWD with OpenOCD and GDB. `tools/openocd/bluepill_stlink.cfg` selects the ST-Link interface, SWD transport, the STM32F1 target, a conservative 1 MHz adapter rate, and `reset_config none`. That reset policy is intentional for boards where NRST is not wired to the probe.

A typical two-terminal session is:

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

The checked-in GDB command file connects to `localhost:3333`, halts/resets the target, loads the ELF, sets a breakpoint at `main`, and continues. The ELF retains source-level debug information because the default optimization is `-Og` with `-g3`.

## How to read each example

Each example contains three first-party documentation entry points:

- `README.md` — what the project does, hardware, exact configuration, mechanism, validation, and trade-offs;
- `docs/architecture.md` — ownership, initialization, data flow, concurrency, invariants, and failure propagation;
- `docs/porting_guide.md` — what must change when pins, clocks, buses, devices, or MCU family change.

Use the README first, then architecture, then porting. The source remains authoritative when behavior and documentation ever disagree.

## References

- [STMicroelectronics — STM32F103 documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32f103/documentation.html)
- [STMicroelectronics — RM0008: STM32F101/102/103/105/107 reference manual](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [STMicroelectronics — PM0056: STM32F10xxx Cortex-M3 programming manual](https://www.st.com/resource/en/programming_manual/pm0056-stm32f10xxx20xxx21xxxl1xxxx-cortexm3-programming-manual-stmicroelectronics.pdf)
- [Arm — CMSIS Core documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [GNU Binutils — linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)
- [OpenOCD documentation](https://openocd.org/pages/documentation.html)
- [GDB — remote debugging](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Debugging.html)
