# 04-uart-interrupt-ring-buffer — USART1 Interrupt + RX/TX Ring Buffers

## 1. Learning Objectives

This example upgrades Example 03 from polling-only byte transfer to
interrupt-driven buffering.

You will learn:

- USART1 RXNE/TXE interrupts;
- static single-producer/single-consumer ring buffers;
- RX ISR producer vs thread consumer;
- thread TX producer vs ISR consumer;
- TXE interrupt enable/disable lifecycle;
- receive error and overflow counters;
- bounded Application work budget.

## 2. Wiring

Same as Example 03:

```text
PA9  USART1_TX  ---> USB-UART RX
PA10 USART1_RX  <--- USB-UART TX
GND              --- common ground
```

Terminal:

```text
115200 8N1
no flow control
```

## 3. Compile-Time Configuration

```c
#define BOARD_UART_BAUD_RATE      (115200UL)
#define BOARD_UART_IRQ_PRIORITY   (5U)
#define BOARD_UART_RX_BUFFER_SIZE (128U)
#define BOARD_UART_TX_BUFFER_SIZE (128U)
```

Both rings use static storage.

## 4. Ring-Buffer Model

The generic ring buffer stores:

```text
storage pointer
storage_size
head
tail
```

The implementation reserves one slot.

For a 128-byte storage array:

```text
usable capacity = 127 bytes
```

### RX Ring

```text
producer: USART1 ISR
consumer: thread mode
```

### TX Ring

```text
producer: thread mode
consumer: USART1 ISR
```

This is exactly the single-producer/single-consumer contract documented by the
Common ring-buffer module.

## 5. Empty and Full

Empty:

```text
head == tail
```

Full:

```text
next(head) == tail
```

Reserving one slot avoids needing a shared element count.

The implementation uses compiler memory barriers around publication/consumption
of head/tail updates.

## 6. Initialization Flow

`board_uart_init()`:

1. initializes RX ring;
2. initializes TX ring;
3. clears diagnostic counters;
4. enables GPIOA + USART1 clocks;
5. configures PA9/PA10;
6. configures USART1 115200 8N1;
7. sets NVIC priority;
8. enables USART1 IRQ;
9. enables RXNE interrupt;
10. enables USART error interrupt;
11. leaves TXE interrupt disabled;
12. enables USART1.

## 7. RX Interrupt Path

```text
RX byte arrives
    |
USART1_IRQHandler
    |
read SR
    |
RXNE/error?
    |
read DR
    |
push byte to RX ring
    |
overflow?
    |
increment counter
```

Reading SR followed by DR also participates in clearing STM32F1 receive
conditions.

Thread mode later calls:

```c
uart_service_try_read_byte()
```

to pop from the RX ring.

## 8. TX Interrupt Path

Thread mode:

```text
try_write(byte)
    |
push into TX ring
    |
enable TXE interrupt
```

ISR:

```text
TXE set + TXEIE enabled?
    |
pop TX ring
    |
    +--> byte exists -> write DR
    |
    +--> empty -> disable TXEIE
```

Disabling TXEIE when the ring becomes empty prevents an interrupt storm.

## 9. Thread-Mode Write and TX-Start Race

The producer pushes the byte before enabling TXEIE.

If TXE is already set, enabling TXEIE immediately makes the USART interrupt
eligible and transmission starts.

The TX ring's producer/consumer ownership prevents both contexts from updating
the same index.

## 10. Thread-Mode Read

Thread mode only consumes the RX ring.

The ISR only produces it.

This separation avoids a general-purpose lock around every byte.

## 11. RX Overflow Semantics

If the RX ring is full when a new byte arrives:

```text
new byte is discarded
s_rx_overflow_count++
```

The counter makes data loss observable.

No heap or dynamic ring expansion is attempted.

## 12. Hardware Error Flags

The ISR tracks:

```text
ORE  overrun error
NE   noise error
FE   framing error
PE   parity error
```

Any receive error increments:

```c
s_rx_error_count
```

This counter is distinct from software ring overflow.

## 13. Application Behavior

The Application first queues the greeting:

```text
STM32F103 UART interrupt + ring buffer ready
Type characters to echo.
```

It uses:

```c
#define APPLICATION_PROCESS_BUDGET (32U)
```

so one call cannot process an unlimited amount of UART work.

After the greeting it echoes bytes.

If TX temporarily becomes full, one received byte is kept in
`s_echo_pending` until it can be queued.

## 14. Debug Symbols

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

## 15. Interrupt Ownership

`USART1_IRQHandler()` belongs to Board UART.

The ISR:

- transfers bytes;
- updates low-level counters;
- enables/disables TXE interrupt.

It does not:

- echo directly;
- call UART Service;
- call Application;
- parse text.

## 16. Architecture

```text
Application
    |
UART Service
    |
Board UART
    |
    +--> RX ring <---- USART1 IRQ
    +--> TX ring ----> USART1 IRQ
    |
USART/GPIO/RCC SPL
```

The ring-buffer implementation lives in Common.

## 17. Comparison with Example 03

Example 03:

```text
thread polls hardware directly through BSP
```

Example 04:

```text
ISR moves bytes between hardware and rings
thread consumes/produces rings
```

The Application-facing byte API remains non-blocking.

## 18. Idle Behavior

The example still uses `__NOP()`.

UART transfer continues through interrupts even while thread mode is between
Application calls.

## Build, Flash, and Debug

```bash
make check-layers
make clean
make
make flash
```

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

## 19. Stress Test

1. Open 115200 8N1 terminal.
2. Verify greeting.
3. Paste a large block of text.
4. Observe echo.
5. Inspect `s_rx_overflow_count`.
6. Intentionally halt the MCU briefly while the sender continues.
7. Resume and observe overflow/error diagnostics.

## 20. Troubleshooting

### No Greeting

Check USART initialization, PA9, NVIC, TX ring state, and whether TXEIE becomes
enabled after the first queued byte.

### Greeting Stops After the First Byte

Check:

- `USART1_IRQHandler()` is reached;
- TXE remains enabled while data exists;
- ISR pops the TX ring;
- TXEIE is not disabled prematurely.

### RX Overflow Increases

The Application is not consuming RX quickly enough relative to the sender.

Options:

- increase ring size;
- reduce input rate;
- improve Application budget;
- add flow control at a higher design level.

### Hardware Overrun/Error Increases

Check baud mismatch, signal quality, host behavior, and whether interrupts are
blocked too long elsewhere.

## 21. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
