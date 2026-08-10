# Porting Guide — 03-uart-polling

## 1. Changing Pins While Keeping USART1

STM32F1 alternate-function mapping may require AFIO remap support if you move
away from the default PA9/PA10 mapping.

Update BSP only.

Application and UART Service should remain unchanged.

## 2. Moving to USART2/USART3

Update:

- peripheral instance;
- RCC bus/clock;
- TX/RX pins;
- GPIO port;
- any remap configuration.

Remember USART1 is on APB2, while USART2/USART3 are on APB1.

## 3. Changing Baud Rate

Update:

```c
#define BOARD_UART_BAUD_RATE (...)
```

SPL recalculates the peripheral baud configuration.

Verify with a terminal or logic analyzer.

## 4. Changing the Clock Tree

Re-check the peripheral clock seen by the selected USART.

Do not assume baud remains correct after changing system/APB clocks.

## 5. Changing Data Format

Modify BSP `USART_InitTypeDef`:

- word length;
- parity;
- stop bits;
- hardware flow control.

Document matching terminal settings.

## 6. Adding a Blocking API with Timeout

If a blocking helper is required, keep it below Application and make the wait
bounded.

Prefer preserving the non-blocking API for normal super-loop use.

## 7. Porting to Another MCU Family

Keep:

```text
Application -> UART Service
```

Replace the Board UART implementation and vendor peripheral layer.

## 8. Post-Port Tests

- [ ] TX idle level correct.
- [ ] greeting readable.
- [ ] exact 115200 baud verified.
- [ ] RX path works.
- [ ] echo works.
- [ ] no blocking wait was accidentally introduced.
- [ ] layer checker passes.

## 9. Common Pitfalls

- TX connected to TX instead of adapter RX;
- no common ground;
- 5 V adapter logic;
- wrong APB clock assumption;
- forgetting AF remap;
- changing USART but not GPIO/RCC mapping;
- turning `try_write` into an unbounded wait.
