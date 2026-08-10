# Porting Guide — 04-uart-interrupt-ring-buffer

## 1. Parts That Can Remain Unchanged

When only the physical UART changes, these parts can normally remain:

```text
app/
services/
common/byte_ring_buffer
```

Keep the same logical contract:

```text
try_read
try_write
can_read
can_write
```

Replace only the Board UART implementation and configuration.

## 2. Changing the USART Instance

Moving from USART1 to USART2/USART3 requires review of:

- peripheral instance;
- RCC bus/clock;
- GPIO pins;
- alternate-function/remap;
- IRQ number;
- strong handler name.

Remember USART1 is on APB2 while USART2/USART3 are on APB1.

## 3. Changing Buffer Size

Update:

```c
BOARD_UART_RX_BUFFER_SIZE
BOARD_UART_TX_BUFFER_SIZE
```

The ring reserves one slot, so usable capacity is:

```text
N - 1
```

Check total SRAM usage after increasing buffers.

## 4. Changing IRQ Priority

Review the priority relative to:

- SysTick;
- DMA;
- EXTI;
- other communication peripherals.

Priority does not justify a long ISR. Keep byte movement bounded.

## 5. Changing Baud/Data Format

Update:

```text
baud
word length
parity
stop bits
flow control
```

in the BSP USART configuration.

Verify the host terminal uses matching settings.

## 6. Changing TX/RX Pins

Update BSP pin mapping.

If alternate-function remap is required, configure AFIO.

Do not expose pin changes to UART Service/Application.

## 7. Porting to DMA UART

Preserve the upper API if possible:

```text
Application -> UART Service
```

Replace the Board UART data movement with DMA/ring/block logic.

Define clearly:

- DMA channel ownership;
- TX completion semantics;
- RX circular-buffer handoff;
- overflow policy.

## 8. Concurrency Validation

Verify the single-producer/single-consumer contract:

RX:

```text
ISR/DMA producer
thread consumer
```

TX:

```text
thread producer
ISR/DMA consumer
```

Do not add a second producer without redesigning synchronization.

Stress-test ring wraparound and temporary thread stalls.

## 9. Symbol Validation

After porting, verify the intended strong handler is linked.

Examples:

```text
USART2_IRQHandler
USART3_IRQHandler
```

The old `USART1_IRQHandler` should no longer be the active owner if USART1 is
not used.

Use the ELF/map/symbol table as needed.

## 10. Common Pitfalls

- changing USART instance but not IRQ handler name;
- wrong APB clock assumption;
- forgetting AFIO remap;
- ring storage size interpreted as usable capacity;
- leaving TXE interrupt permanently enabled;
- two producers modifying one ring;
- losing bytes without an overflow counter;
- moving protocol parsing into the ISR.
