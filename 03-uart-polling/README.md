# 03 - UART Polling

## Purpose

Configure USART1 at 115200 8N1 and implement a cooperative polling echo without interrupts, DMA, blocking delays, or unbounded waits.

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


Use a 3.3 V USB-to-UART adapter:

```text
Blue Pill PA9  USART1_TX  ---> adapter RX
Blue Pill PA10 USART1_RX  <--- adapter TX
Blue Pill GND              --- adapter GND
```

Terminal settings:

```text
115200 baud
8 data bits
no parity
1 stop bit
no flow control
```


## Compile-Time Configuration


`BOARD_UART_BAUD_RATE` is `115200`.

Board mapping:

```text
USART1_TX = PA9
USART1_RX = PA10
```

TX uses alternate-function push-pull at 50 MHz. RX is floating input.


## Initialization Sequence


```text
board_init()
    |
    +--> SystemCoreClockUpdate()
    +--> board_uart_init()
            |
            +--> GPIOA + USART1 clocks
            +--> PA9 AF push-pull
            +--> PA10 floating input
            +--> USART 115200 8N1
            +--> RX + TX enable

system_init()
    |
    +--> uart_service_init()
    +--> application_init()
```

`uart_service_init()` has no hardware work because the BSP already owns
peripheral initialization.


## Runtime Behavior


The Application first sends:

```text
STM32F103 UART polling ready
Type characters to echo.
```

The message is not transmitted by a blocking string function. Each
`application_process()` call attempts one byte when `TXE` is ready.

After the greeting, the state machine keeps at most one pending echo byte:

```text
RXNE ready?
    |
    +--> read byte
          |
          v
      pending echo
          |
TXE ready?
    |
    +--> write byte
```

If hardware is not ready, the function simply returns to the super-loop.


## SPL / Low-Level Behavior


`board_uart_try_read_byte()` checks `USART_FLAG_RXNE`.

`board_uart_try_write_byte()` checks `USART_FLAG_TXE`.

There is no USART IRQ, no DMA, and no software queue. This makes the example a
useful baseline for comparison with Example 04.


## Architectural Notes


The key lesson is API shape. The upper layers already use `try_read` and
`try_write`, which means Example 04 can change the transport implementation to
interrupt-driven rings while preserving a similar non-blocking Application
model.


## Interrupt and Concurrency Policy

The project follows the repository-wide rule that an interrupt handler belongs
to the lowest module that owns the peripheral. The ISR, when present, may clear
flags, transfer low-level data, and record bounded state. Higher-level policy is
processed later in normal thread mode.

`system_idle()` in this concrete example executes `__NOP()` rather than
`__WFI()`.

## Test Procedure and Expected Result


Open a serial terminal at 115200 8N1 and reset the board.

Expected greeting:

```text
STM32F103 UART polling ready
Type characters to echo.
```

Type characters. Every received byte should be transmitted back.

If there is no greeting:

1. verify TX/RX are crossed correctly;
2. verify common ground;
3. verify the adapter uses 3.3 V logic;
4. break in `board_uart_try_write_byte()`;
5. inspect whether `USART_FLAG_TXE` becomes set.

If greeting works but echo does not, inspect PA10/RX and `USART_FLAG_RXNE`.


## GDB Debugging


```gdb
break board_uart_try_read_byte
break board_uart_try_write_byte
break application_process
continue
```

Application state can be inspected in context:

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


To move the console, update the USART instance, RCC clocks, GPIO port, and TX/RX
pins in the BSP. If moving to USART2/USART3, also verify APB clock selection and
alternate-function pin mapping.


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
