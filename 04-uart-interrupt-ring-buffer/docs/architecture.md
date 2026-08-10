# Architecture — 04-uart-interrupt-ring-buffer

## 1. Dependency Graph

```text
Application
    |
UART Service
    |
Board UART
    |
    +--> Common RX ring
    +--> Common TX ring
    |
USART1 SPL + CMSIS NVIC
```

## 2. Ownership

| State/resource | Producer | Consumer | Owner |
|---|---|---|---|
| USART1 RX data | hardware/ISR | RX ring | Board UART |
| RX ring | ISR | thread | Board UART/Common |
| TX ring | thread | ISR | Board UART/Common |
| RX error counter | ISR | debug/thread | Board UART |
| overflow counter | ISR | debug/thread | Board UART |
| echo policy | thread | — | Application |

## 3. Ring-Buffer Concurrency Contract

The Common ring requires exactly one producer and one consumer.

RX:

```text
ISR -> head
thread -> tail
```

TX:

```text
thread -> head
ISR -> tail
```

This avoids two contexts updating the same index.

## 4. RX Data Lifecycle

```text
USART DR
    |
ISR reads byte
    |
RX ring
    |
UART Service
    |
Application
```

If the ring is full, the newest arriving byte is discarded and overflow is
counted.

## 5. TX Data Lifecycle

```text
Application
    |
UART Service
    |
TX ring
    |
TXE interrupt
    |
USART DR
```

The producer never waits for one hardware byte to physically finish before
queueing the next available byte.

## 6. TXE Interrupt Lifecycle

TXE interrupt is demand-driven:

```text
ring receives data -> enable TXEIE
ring becomes empty -> disable TXEIE
```

Leaving TXEIE enabled while TXE remains asserted would repeatedly re-enter the
handler.

## 7. Hardware Error Handling

Receive errors are captured at the same low-level point where SR/DR are read.

The board layer converts hardware status into counters rather than pushing raw
USART status to Application.

## 8. Memory Ordering

The ring implementation uses compiler memory barriers around:

- publishing a new head;
- observing producer head before reading data;
- publishing a new tail.

The barriers prevent compiler reordering from violating the intended
producer/consumer sequence.

## 9. Application Processing Budget

The fixed budget prevents UART work from monopolizing thread mode.

This is a fairness mechanism, not a hardware requirement.

A larger application can apply similar budgets to other Services.

## 10. Failure/Overflow Model

The example favors bounded behavior:

- ring full -> return `false`;
- RX overflow -> drop + count;
- UART receive error -> count;
- no dynamic allocation;
- no blocking wait for space.

## 11. ISR Boundary

Correct:

```text
ISR -> rings/counters -> thread
```

Incorrect:

```text
ISR -> echo policy -> Application
```

## 12. Extension Strategy

Possible extensions:

- line-oriented receive Service;
- packet framing;
- larger rings;
- flow control;
- DMA-backed UART.

Preserve the board-level byte transport boundary when adding protocol logic.
