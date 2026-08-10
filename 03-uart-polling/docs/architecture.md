# Architecture — 03-uart-polling

## 1. Dependency Graph

```text
Application
    |
    v
UART Service
    |
    v
Board UART
    |
    v
GPIO/RCC/USART SPL
```

## 2. Responsibilities

**Application**

Owns greeting and echo state.

**UART Service**

Provides a hardware-independent byte API.

**Board UART**

Owns USART1, PA9, PA10, baud configuration, and peripheral initialization.

## 3. Public Contract

The important contract is:

```c
bool uart_service_try_read_byte(uint8_t *byte);
bool uart_service_try_write_byte(uint8_t byte);
```

`false` means "not completed now", not necessarily a fatal error.

This API shape supports cooperative thread-mode code.

## 4. Polling Ownership

Polling occurs in the BSP because only the BSP knows USART1 status flags.

Application does not read RXNE/TXE directly.

## 5. Error Handoff

The current Example 03 API intentionally does not distinguish:

- no byte available;
- detailed USART receive errors.

This keeps the baseline simple.

Example 04 expands the low-level ownership with explicit error/overflow
diagnostics.

## 6. No Interrupt Concurrency

There is no USART ISR concurrency.

UART state is accessed only from thread mode.

This makes the example useful for understanding the basic peripheral before
introducing rings and ISR ownership.

## 7. Timing Dependency

The USART baud calculation depends on PCLK2.

SPL handles BRR calculation, but the BSP still owns the responsibility to
configure the correct clock tree before USART initialization.

## 8. Initialization Dependency

```text
System clock information
    |
Board UART
    |
UART Service
    |
Application
```

Application can send the greeting only after USART1 is enabled.

## 9. Layer Boundary

Application knows:

```text
read byte
write byte
```

It does not know:

```text
USART1
PA9/PA10
TXE/RXNE
PCLK2
BRR
```

## 10. Failure Path

`board_uart_init()` is void in this example, so runtime bring-up failures are
diagnosed by behavior/GDB rather than propagated through a status return.

A future production-oriented wrapper could validate configuration and return
`bool`, while preserving the same upper-layer API.
