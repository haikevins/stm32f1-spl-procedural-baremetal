# UART Polling — Non-Blocking USART1

> **Scope:** Example 3 of the repository progression — USART1 115200 8N1 with `try_read`/`try_write`, no IRQ/DMA, one-byte pending echo state.

[Main](https://github.com/haikevins/stm32f1-spl-procedural-baremetal) · [↑ Examples](../README.md) · [← Previous](../02-gpio-input-interrupt/README.md) · [Next →](../04-uart-interrupt-ring-buffer/README.md) · [Architecture](docs/architecture.md) · [Porting](docs/porting_guide.md)

## Table of contents

- [Purpose and expected behavior](#purpose-and-expected-behavior)
- [Hardware and wiring](#hardware-and-wiring)
- [Configuration](#configuration)
- [Source ownership](#source-ownership)
- [Runtime flow](#runtime-flow)
- [Mechanism in depth](#mechanism-in-depth)
- [Concurrency and ownership](#concurrency-and-ownership)
- [Initialization and failure behavior](#initialization-and-failure-behavior)
- [Build, flash, and debug](#build-flash-and-debug)
- [Design decisions and limitations](#design-decisions-and-limitations)
- [Porting boundary](#porting-boundary)
- [References](#references)

## Purpose and expected behavior

After reset the application emits a startup banner one byte at a time whenever TXE says the transmitter can accept a byte. It then polls RXNE. A received byte is retained in one application-owned pending slot until a later TXE poll succeeds, creating a non-blocking echo without interrupt, DMA, delay loops, or an unbounded wait.

This example remains intentionally small at Application level. The educational value is in the boundary between product policy and the lower-level mechanism: USART1 115200 8N1 with `try_read`/`try_write`, no IRQ/DMA, one-byte pending echo state.

## Hardware and wiring

USART1 uses PA9 as alternate-function push-pull TX and PA10 as floating-input RX. Connect a 3.3 V USB-UART adapter with crossed TX/RX and common ground.

```text
Blue Pill                 USB-UART (3.3 V)
-------------------------------------------
PA9  / USART1_TX  ------> RX
PA10 / USART1_RX  <------ TX
GND                 ----- GND
```

SWD uses PA13/PA14 and common ground. The checked-in OpenOCD setup does not require the probe's NRST signal.

## Configuration

| Setting | Value |
|---|---:|
| Baud | 115200 bit/s |
| Data | 8 bits |
| Parity | none |
| Stop | 1 |
| Flow control | none |
| USART interrupts | disabled |
| DMA | disabled |

Compile-time constants live under `config/`; board mappings live under `bsp/bluepill/`. Application source therefore does not duplicate pin numbers, raw peripheral names, or clock-tree formulas.

## Source ownership

```text
app/src/application.c
  -> uart_service
services/src/uart_service.c
  -> board_uart try-read / try-write
bsp/bluepill/src/board_uart.c
  -> GPIOA + USART1 / SPL
```

The dependency direction is checked by `tools/scripts/check_layers.py`. `system/system_init.c` is the composition root and is allowed to connect the layers; Application is not.

## Runtime flow

```text
STARTUP_TX
    |
    | all banner bytes accepted by TXE polling
    v
RX_WAIT  -- RXNE byte -->  ECHO_PENDING
   ^                         |
   |---- TXE accepts byte ---|
```

Each `application_process()` call performs only immediately available work and then returns. During startup it attempts at most one banner byte. After startup it holds at most one received byte until USART1 can accept it for transmission.

The reset/startup sequence before this flow is common to every example: custom `Reset_Handler` initializes `.data` and `.bss`, calls vendor `SystemInit()`, then project `main()` calls `system_init()` and enters the cooperative loop.

## Mechanism in depth

### Polling contract

`board_uart_try_read_byte()` returns immediately if RXNE is clear. `board_uart_try_write_byte()` returns immediately if TXE is clear. The Service simply exposes this capability upward. This is very different from code that loops until the flag changes: every call has bounded execution time.

### One-byte backpressure

The application does not consume a second RX byte while the first byte is waiting for TX. That creates a minimal ownership invariant: if `s_echo_pending` is true, `s_echo_byte` is the one byte that must be transmitted before new receive data is accepted.

### Hardware versus software buffering

There is no software RX queue. USART hardware provides only the peripheral data path, so a new character can be lost/overrun if the super-loop does not read DR quickly enough. This limitation is exactly what Example 04 addresses using interrupts and ring buffers.

### Clock dependency

USART1 is on APB2. SPL derives its baud configuration from the peripheral clock assumptions established by the vendor clock tree; changing clocks requires verifying the actual baud error rather than retaining a copied constant.

## Concurrency and ownership

This example is intentionally thread-only for USART. The super-loop is the sole reader/writer; there is no shared UART state with an ISR. That makes it a baseline for understanding why the ownership model changes in Example 04.

The general repository rule still holds: the lowest layer that owns an interrupt source acknowledges/publishes hardware state, while Services/Application consume that state outside the ISR unless a truly low-level bounded operation is required.

## Initialization and failure behavior

Initialization order:

```text
board USART1 → UART Service → Application
```

The current API does not surface framing/parity/noise/overrun diagnostics to Application. It demonstrates readiness polling, not a production serial error-recovery channel.

`main()` treats a failed `system_init()` as fatal and calls `system_panic()`, which disables interrupts and remains in a debug-friendly halt loop.

## Build, flash, and debug

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

For this example, inspect its public Application/Service variables and peripheral registers in GDB rather than adding unrelated logging dependencies simply for observation.

## Design decisions and limitations

- Very small state and simple ownership, but latency depends on super-loop frequency.
- No software receive buffering; sustained input can outrun thread-mode polling.
- One-byte-per-process behavior for the startup banner intentionally preserves cooperative scheduling instead of maximizing throughput.

These are documented constraints of the example, not claims that the mechanism is universally optimal.

## Porting boundary

When changing UART instance/pins, verify APB bus, AF pin modes/remap, peripheral clock, baud error, and voltage levels. If moving to interrupt/DMA operation, the public Service can remain non-blocking while the BSP implementation gains buffering.

See [the detailed porting guide](docs/porting_guide.md) for the change matrix and validation order.

## References

- [STMicroelectronics — STM32F103 documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32f103/documentation.html)
- [STMicroelectronics — RM0008: STM32F101/102/103/105/107 reference manual](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [STMicroelectronics — PM0056: STM32F10xxx Cortex-M3 programming manual](https://www.st.com/resource/en/programming_manual/pm0056-stm32f10xxx20xxx21xxxl1xxxx-cortexm3-programming-manual-stmicroelectronics.pdf)
- [Arm — CMSIS Core documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [GNU Binutils — linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)
- [OpenOCD documentation](https://openocd.org/pages/documentation.html)
- [GDB — remote debugging](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Debugging.html)


---

[Main](https://github.com/haikevins/stm32f1-spl-procedural-baremetal) · [↑ Examples](../README.md) · [← Previous](../02-gpio-input-interrupt/README.md) · [Next →](../04-uart-interrupt-ring-buffer/README.md) · [Architecture](docs/architecture.md) · [Porting](docs/porting_guide.md)
