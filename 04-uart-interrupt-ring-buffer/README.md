# Example 04: UART Interrupt with Ring Buffers

A layered STM32F103C8T6 Blue Pill example using CMSIS and the STM32F10x
Standard Peripheral Library.

USART1 is configured for 115200 baud, 8 data bits, no parity, and one stop
bit. RXNE and TXE interrupts move bytes between USART1 and two
single-producer/single-consumer ring buffers. Application code performs the
echo outside interrupt context.

## Hardware wiring

Use a 3.3 V USB-to-UART adapter.

```text
Blue Pill PA9  (USART1_TX) ---- USB-to-UART RX
Blue Pill PA10 (USART1_RX) ---- USB-to-UART TX
Blue Pill GND                 ---- USB-to-UART GND
```

Do not connect 5 V logic signals to PA9 or PA10.

| Resource | Configuration |
|---|---|
| Peripheral | USART1 |
| TX | PA9, alternate-function push-pull |
| RX | PA10, floating input |
| Baud rate | 115200 |
| Frame | 8 data bits, no parity, 1 stop bit |
| Flow control | None |
| RX method | RXNE interrupt + ring buffer |
| TX method | TXE interrupt + ring buffer |
| DMA | Disabled |
| RX storage | 128 bytes, 127-byte usable capacity |
| TX storage | 128 bytes, 127-byte usable capacity |

## Expected behavior

After reset, the terminal displays:

```text
STM32F103 UART interrupt + ring buffer ready
Type characters to echo.
```

Every received byte is echoed unchanged. The RX interrupt can continue
capturing a burst while the Application processes previously received data.

Configure the terminal for 115200 baud, 8-N-1, no flow control:

```bash
picocom -b 115200 /dev/ttyUSB0
```

## Dependency path

```text
Application
    -> UART Service
        -> Board UART
            -> Byte Ring Buffer
            -> GPIO / RCC / USART1 / NVIC
```

Application code does not include BSP, SPL, CMSIS, or STM32 device headers.

## Interrupt responsibilities

`USART1_IRQHandler()` performs bounded work only:

```text
RXNE/error:
    read SR and DR
    push one valid byte into RX ring
    update overflow/error counters

TXE:
    pop one byte from TX ring
    write DR
    disable TXE interrupt when the TX ring becomes empty
```

The ISR does not parse commands, echo bytes, wait, allocate memory, or call
Application code.

## Ring-buffer concurrency

The generic byte ring buffer is single-producer/single-consumer:

```text
RX ring:
    producer = USART1 ISR
    consumer = main/Application

TX ring:
    producer = main/Application
    consumer = USART1 ISR
```

Producer and consumer own different indices, so no global interrupt masking
is required around normal push/pop operations. One storage slot is reserved
to distinguish full from empty.

## Race-free WFI idle

This example can sleep because USART1 interrupts wake the core. It also
avoids the common lost-wakeup race:

```c
__disable_irq();

if (!application_has_pending_work())
{
    __DSB();
    __WFI();
}

__enable_irq();
__ISB();
```

An interrupt that becomes pending while `PRIMASK` is set wakes `WFI`; after
interrupts are enabled, its handler runs. This prevents a byte arriving
between the final work check and `WFI` from remaining unprocessed until a
second interrupt occurs.

## Overflow and error counters

The BSP records:

```c
uint32_t board_uart_get_rx_overflow_count(void);
uint32_t board_uart_get_rx_error_count(void);
```

Overflow means the RX ISR received a valid byte while the RX ring was full.
Error count includes overrun, noise, framing, and parity status observed by
the ISR. The example exposes both values through UART Service for debugger
inspection.

## Build

```bash
make check-layers
make clean
make
```

## Flash

```bash
make flash
```

## Debug

Terminal 1:

```bash
make debug-server
```

Terminal 2:

```bash
make debug
```

## Configuration

- Baud rate, IRQ priority, buffer sizes: `config/board_config.h`
- USART pin mapping: `bsp/bluepill/src/board_pins.h`
- Selected SPL modules: `config/modules.mk`

## Selected SPL modules

```text
stm32f10x_gpio.c
stm32f10x_rcc.c
stm32f10x_usart.c
```

## License

This example is licensed under the MIT License.
