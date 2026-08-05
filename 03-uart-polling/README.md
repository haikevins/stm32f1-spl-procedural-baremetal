# Example 03: UART Polling

A layered STM32F103C8T6 Blue Pill example using CMSIS and the STM32F10x
Standard Peripheral Library.

USART1 is configured for 115200 baud, 8 data bits, no parity, and one stop
bit. The firmware sends a startup message and echoes every received byte.
Both receive and transmit paths use non-blocking polling; UART interrupts and
DMA are not enabled.

## Hardware wiring

Use a 3.3 V USB-to-UART adapter.

```text
Blue Pill PA9  (USART1_TX) ---- USB-to-UART RX
Blue Pill PA10 (USART1_RX) ---- USB-to-UART TX
Blue Pill GND                 ---- USB-to-UART GND
```

Do not connect 5 V logic signals to PA9 or PA10. A basic ST-Link V2 clone is
a debug probe and normally does not provide a USB virtual COM port, so a
separate USB-to-UART adapter is required.

| Resource | Configuration |
|---|---|
| Peripheral | USART1 |
| TX | PA9, alternate-function push-pull |
| RX | PA10, floating input |
| Baud rate | 115200 |
| Frame | 8 data bits, no parity, 1 stop bit |
| Flow control | None |
| Transfer method | Polling |
| UART interrupts | Disabled |
| DMA | Disabled |

## Expected behavior

After reset, the terminal displays:

```text
STM32F103 UART polling ready
Type characters to echo.
```

Every byte typed in the terminal is transmitted back unchanged. Open the
terminal first and press the Blue Pill Reset button to see the startup
message again.

This introductory example has no software FIFO. Normal typing is handled
comfortably, but a large pasted burst can overrun the single-byte USART data
register. A later interrupt-driven ring-buffer example addresses that case.

Configure the terminal for:

```text
115200 baud
8 data bits
no parity
1 stop bit
no hardware flow control
```

For example on Linux:

```bash
picocom -b 115200 /dev/ttyUSB0
```

## Dependency path

```text
Application
    -> UART Service
        -> Board UART
            -> GPIO / RCC / USART1
```

Application code does not include BSP, SPL, CMSIS, or STM32 device headers.

## Polling policy

The BSP exposes two non-blocking primitives:

```c
bool board_uart_try_read_byte(uint8_t *byte);
bool board_uart_try_write_byte(uint8_t byte);
```

They inspect `RXNE` and `TXE` and return immediately. The Application keeps at
most one echo byte pending, so it never waits inside an unbounded polling
loop.

Because UART interrupts are disabled, this example deliberately uses
`__NOP()` in `system_idle()`. Replacing it with `__WFI()` would stop the
super-loop from polling `RXNE`; incoming UART data alone would not wake the
core.

## Build

```bash
make
```

The build first checks the project dependency rules:

```bash
make check-layers
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

- Baud rate: `config/board_config.h`
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
