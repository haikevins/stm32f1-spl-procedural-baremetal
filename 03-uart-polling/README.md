# 03-uart-polling — USART1 Polling with a Non-Blocking API

## 1. Learning Objectives

This example introduces a byte-stream peripheral without interrupts.

You will learn:

- USART1 TX/RX GPIO setup;
- 115200 8N1 configuration with SPL;
- RXNE/TXE polling;
- non-blocking `try_read` / `try_write`;
- a small Application state machine for greeting + echo;
- why polling APIs should return when hardware is not ready.

## 2. Wiring

Use a 3.3 V USB-to-UART adapter:

```text
Blue Pill PA9  USART1_TX  ---> adapter RX
Blue Pill PA10 USART1_RX  <--- adapter TX
Blue Pill GND              --- adapter GND
```

Terminal:

```text
115200 baud
8 data bits
no parity
1 stop bit
no flow control
```

## 3. Expected Behavior

After reset:

```text
STM32F103 UART polling ready
Type characters to echo.
```

Typed bytes are echoed back.

The greeting and echo are implemented without a blocking transmit loop.

## 4. Compile-Time Configuration

`config/board_config.h`:

```c
#define BOARD_UART_BAUD_RATE (115200UL)
```

BSP mapping:

```text
USART1
TX -> PA9
RX -> PA10
```

## 5. Initialization Flow

```text
board_init()
    |
    +--> SystemCoreClockUpdate()
    +--> board_uart_init()
            |
            +--> enable GPIOA + USART1 clocks
            +--> configure PA9 TX
            +--> configure PA10 RX
            +--> USART 115200 8N1
            +--> enable RX and TX
            +--> enable USART1

system_init()
    |
    +--> uart_service_init()
    +--> application_init()
```

## 6. GPIO Configuration

### TX — PA9

PA9 is configured as:

```text
GPIO_Mode_AF_PP
GPIO_Speed_50MHz
```

USART1 drives the pin through the alternate-function output.

### RX — PA10

PA10 is configured as:

```text
GPIO_Mode_IN_FLOATING
```

The USB-UART adapter drives the RX logic level.

## 7. Baud-Rate Register

This project lets SPL calculate the USART baud configuration from the current
peripheral clock and requested baud rate.

The important architecture point is that Application specifies only the logical
baud configuration through project config; it does not calculate BRR or know
PCLK2.

When changing the clock tree, verify the actual baud with a terminal or logic
analyzer.

## 8. USART Setup

`USART_StructInit()` provides defaults, then the BSP explicitly sets:

```text
baud: 115200
word length: 8 bits
stop bits: 1
parity: none
hardware flow control: none
mode: RX + TX
```

Then:

```c
USART_Init(...);
USART_Cmd(..., ENABLE);
```

## 9. Polling Receive Path

```text
Application
    |
UART Service
    |
board_uart_try_read_byte()
    |
RXNE set?
    |
    +--> no  -> false
    |
    +--> yes -> USART_ReceiveData()
                return byte
```

There is no wait loop.

## 10. Polling Transmit Path

```text
Application
    |
UART Service
    |
board_uart_try_write_byte()
    |
TXE set?
    |
    +--> no  -> false
    |
    +--> yes -> USART_SendData()
                true
```

The Application returns to the super-loop whenever TX hardware is not ready.

## 11. Application State Machine

First phase: send the greeting one byte at a time.

```text
startup_message_index < message_length?
    |
    +--> try_write(current byte)
            |
            +--> success -> advance index
            +--> busy    -> return
```

Second phase: echo.

```text
no pending echo?
    |
    +--> try_read
           |
           +--> byte -> save it, mark pending

pending echo?
    |
    +--> try_write
           |
           +--> success -> clear pending
```

## 12. Why `s_echo_pending` Is Required

RX and TX readiness are independent.

A byte may be received while TXE is not ready.

Without a pending state, the Application would either:

- block waiting for TXE, or
- lose the received byte.

One pending byte bridges the two non-blocking operations.

## 13. Error Mapping

This minimal polling example does not expose detailed USART error counters.

That omission is intentional so the example focuses on basic polling.

Example 04 adds explicit receive error and overflow handling.

## 14. Debug Symbols

Useful Application state:

```text
s_startup_message_index
s_echo_pending
s_echo_byte
```

Useful breakpoints:

```gdb
break board_uart_try_read_byte
break board_uart_try_write_byte
break application_process
```

## 15. Interrupt Policy

USART1 interrupts are not enabled.

`USART1_IRQHandler` remains the weak startup default.

This makes the polling model easy to compare with Example 04.

## 16. Idle Behavior

The example uses `__NOP()` in `system_idle()`.

The super-loop therefore polls frequently without sleeping.

## 17. Architecture

```text
Application
    |
UART Service
    |
Board UART
    |
USART/GPIO/RCC SPL
```

Application never includes an SPL header.

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

## 18. Test Procedure

1. Wire the USB-UART adapter.
2. Open a 115200 8N1 terminal.
3. Reset the MCU.
4. Verify the greeting.
5. Type single characters.
6. Verify each character is echoed.
7. Paste a short string and observe polling limitations.

## 19. Troubleshooting

### No Greeting

Check:

- TX/RX wiring is crossed correctly;
- common ground;
- adapter voltage level;
- PA9 mode;
- USART1 clock;
- TXE status.

### Greeting Is Garbage

Check:

- terminal baud;
- system/PCLK2 clock;
- `BOARD_UART_BAUD_RATE`;
- 8N1 settings.

### Typing Does Not Echo

If greeting works, TX is already proven.

Focus on:

- PA10 wiring;
- adapter TX;
- RXNE;
- `board_uart_try_read_byte()`.

### Bytes Are Lost During Fast Input

That is an expected limitation of this polling/no-buffer design.

Use Example 04 when bursts must be retained asynchronously.

## 20. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
