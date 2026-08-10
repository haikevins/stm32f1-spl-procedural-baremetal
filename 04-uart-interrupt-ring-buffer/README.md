# 04 - UART Interrupt + Ring Buffer

## Purpose

Move USART1 byte transfer into an interrupt handler while keeping Application thread-mode logic non-blocking through static RX/TX ring buffers.

This project is independently buildable and uses the same layered architecture
as the rest of the repository.

## Learning Goals

By the end of this example, you should be able to:

- trace initialization from `main()` through System, BSP, Services, and
  Application;
- identify which layer owns each physical peripheral;
- explain the runtime data/control flow;
- distinguish ISR work from thread-mode work where interrupts are used;
- modify compile-time configuration without violating dependency direction;
- debug the example from the hardware layer upward.

## Hardware and Wiring


The wiring and terminal settings are the same as Example 03:

```text
PA9  USART1_TX  ---> USB-UART RX
PA10 USART1_RX  <--- USB-UART TX
GND              --- common GND
```

Use `115200 8N1`, no hardware flow control.


## Compile-Time Configuration


| Setting | Value |
|---|---|
| USART | USART1 |
| Baud | 115200 |
| IRQ priority | 5 |
| RX storage | 128 bytes |
| TX storage | 128 bytes |

The `byte_ring_buffer` implementation uses a head/tail design in which one
storage slot is reserved to distinguish full from empty. With 128 storage
bytes, usable capacity is 127 bytes per ring.


## Initialization Sequence


`board_uart_init()`:

1. initializes RX and TX ring objects;
2. clears error/overflow counters;
3. configures PA9/PA10;
4. configures USART1 115200 8N1;
5. assigns and enables the USART1 NVIC interrupt;
6. enables RXNE and USART error interrupts;
7. leaves TXE interrupt disabled until data is queued;
8. enables USART1.

The Service remains a thin hardware-independent wrapper.


## Runtime Behavior


RX path:

```text
USART RXNE
    |
USART1_IRQHandler
    |
    +--> read DR
    +--> push byte into RX ring
    +--> record error/overflow counters
    |
thread mode
    |
uart_service_try_read_byte()
```

TX path:

```text
Application/Service try_write
    |
push into TX ring
    |
enable TXE interrupt
    |
USART1_IRQHandler
    |
pop TX byte -> DR
    |
ring empty?
    |
    +--> disable TXE interrupt
```

Application uses a fixed processing budget of 32 operations per
`application_process()` call. This prevents one busy UART stream from owning
the super-loop forever.


## SPL / Low-Level Behavior


The ISR reads the USART status register once and handles receive/error state and
TXE state.

Receive error bits counted are:

- ORE;
- NE;
- FE;
- PE.

An RX byte that arrives while the RX ring is full increments the overflow
counter.

TXE interrupt is disabled when the transmit ring becomes empty, preventing an
interrupt storm while TXE remains asserted.


## Architectural Notes


This example makes ISR/thread ownership visible. The UART BSP owns the rings and
the handler; upper layers never access USART registers or NVIC directly.

The generic byte ring buffer lives in `common/`, so it can be reused by other
drivers without depending on UART or STM32.


## Interrupt and Concurrency Policy

The project follows the repository-wide rule that an interrupt handler belongs
to the lowest module that owns the peripheral. The ISR, when present, may clear
flags, transfer low-level data, and record bounded state. Higher-level policy is
processed later in normal thread mode.

`system_idle()` in this concrete example executes `__NOP()` rather than
`__WFI()`.

## Test Procedure and Expected Result


Open a terminal at 115200 8N1.

Expected greeting:

```text
STM32F103 UART interrupt + ring buffer ready
Type characters to echo.
```

Paste or type bursts of text. Echo should continue while the background ISR
moves bytes.

For a stress test, send data faster than the Application can consume and inspect
the RX overflow counter.


## GDB Debugging


```gdb
break USART1_IRQHandler
break byte_ring_buffer_push
break byte_ring_buffer_pop
continue
```

Useful BSP state:

```gdb
p s_rx_overflow_count
p s_rx_error_count
p s_rx_ring
p s_tx_ring
```

Useful Application state:

```gdb
p s_startup_message_index
p s_echo_pending
p/x s_echo_byte
```



## Build, Flash, and Debug

Run the commands from the example directory.

```bash
make check-layers
make clean
make
```

The build produces:

```text
build/firmware.elf
build/firmware.hex
build/firmware.bin
build/firmware.lst
build/firmware.map
```

Flash with OpenOCD:

```bash
make flash
```

Erase the MCU flash if needed:

```bash
make erase
```

Start an OpenOCD debug server:

```bash
make debug-server
```

Then, in another terminal:

```bash
make debug
```

The Makefile prefers `arm-none-eabi-gdb` and falls back to `gdb-multiarch`.

The OpenOCD configuration uses SWD and:

```tcl
reset_config none
adapter speed 1000
```

This matches a common ST-Link connection where only `SWDIO`, `SWCLK`, `GND`,
and `3.3V` are connected and NRST is not available.


## Troubleshooting Method

Use a bottom-up approach:

1. verify power and wiring;
2. verify BSP pin/peripheral mapping;
3. verify the peripheral clock is enabled;
4. verify initialization succeeds;
5. verify the low-level peripheral flag/interrupt/data path;
6. verify Service state;
7. verify Application policy.

Do not immediately modify Application code when the underlying peripheral is
not yet proven to work.

## Porting Notes


When moving to another USART, update the physical mapping and IRQ name in the
BSP. Verify the handler name exactly matches the startup vector. Recalculate IRQ
priority policy if the project adds other real-time interrupt sources.


See [`docs/porting_guide.md`](docs/porting_guide.md) for a structured checklist.

## Further Exercises

Good next experiments include:

- expose additional diagnostic counters through GDB;
- add a second logical Service without letting Application include BSP headers;
- deliberately inject a failure and trace how it propagates;
- write a host-side test for portable Common or Service logic;
- change one board resource and verify that Application does not need hardware
  includes.

## Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/adding_a_module.md`](docs/adding_a_module.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
