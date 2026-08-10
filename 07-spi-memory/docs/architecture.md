# Architecture - 07 - SPI W25Q64 Memory

## Runtime Dependency Direction

```text
Application
    |
    v
Services
    |
    +------> BSP
    |
    +------> ECUAL, when an external device exists
                  |
                  v
          Board peripheral APIs
                  |
                  v
            STM32F10x SPL
                  |
                  v
              CMSIS/MCU
```

`system/` is the composition root and owns initialization order.

## Example-Specific Data Flow


The destructive startup test uses the final 4 KiB sector:

```text
0x007FF000 .. 0x007FFFFF
```

Sequence:

```text
JEDEC ID
   |
Sector Erase 0x20
   |
Write Enable + poll WEL
   |
poll BUSY until erase complete
   |
Page Program 0x02, 32 bytes
   |
poll BUSY
   |
Read Data 0x03
   |
byte-for-byte verify
```

If the test passes, PC13 toggles every 500 ms. If a memory operation or
verification fails after initialization, the Application keeps PC13 steadily
ON.


## Module Responsibilities

### Application

Owns demo/product policy. It must not know physical pins, peripheral instances,
SPL structures, or interrupt flags.

### Services

Translate board/external-device capabilities into stable application-facing
APIs. Service logic is where debounce, filtering, aggregation, or logical
indications belong.

### BSP

Owns the Blue Pill mapping, clock enable, GPIO configuration, STM32 peripheral
initialization, NVIC setup, and low-level ISR when applicable.

### ECUAL

Used only when this example communicates with an off-chip device. ECUAL owns
the external device protocol and should depend on a board bus abstraction.

### Common

Contains portable helpers or shared types with no STM32 dependency.

### System

Initializes modules in dependency order and then runs the super-loop. It must
not contain the example's behavior.

## Initialization


```text
board_init()
    |
    +--> board_timebase_init()
    +--> board_led_init()
    +--> board_memory_bus_init()
    |      +--> CS output HIGH
    |      +--> PA5/PA7 AF push-pull
    |      +--> PA6 floating input
    |      +--> select SPI prescaler
    |      +--> SPI1 mode 0
    |
    +--> 10 ms power-on delay

memory_service_init()
    |
    +--> w25q64_init()
            |
            +--> wait BUSY clear
            +--> command 0x9F
            +--> read 3-byte JEDEC ID
```

The driver requires manufacturer `0xEF` and capacity code `0x17`. It records
the memory-type byte but deliberately does not require one exact type value.


## Low-Level Ownership


The SPI BSP is polling. For every byte it:

1. waits for TXE;
2. writes the transmit byte or `0xFF` dummy byte;
3. waits for RXNE;
4. reads the received byte;
5. after the transfer, waits for BSY to clear.

Chip select is controlled explicitly by GPIO around each command transaction.

The ECUAL validates address range and rejects Page Program operations that
cross a 256-byte page boundary.


## Interrupt Boundary

The weak startup vector is overridden only by the module that owns the active
interrupt source.

The correct flow is:

```text
hardware interrupt
    |
lowest owning module ISR
    |
static low-level state
    |
normal thread-mode API
    |
Service
    |
Application
```

The wrong flow is:

```text
ISR -> Application callback/state machine
```

## Concurrency Principles

When state is shared between ISR and thread mode:

- keep the shared object static and bounded;
- mark asynchronously changed scalar state `volatile` where appropriate;
- make multi-step read/clear operations atomic with a short critical section;
- never hold interrupts disabled while performing slow peripheral operations;
- make overflow/error behavior explicit.

## Why This Separation Matters


The Memory Service hides the concrete W25Q64 device from Application. The ECUAL
owns NOR command semantics. The BSP owns SPI1 and CS pin transactions. This
keeps board wiring and flash protocol concerns separate.


## Dependency Enforcement

Run:

```bash
make check-layers
```

A successful build should not require weakening the checker. If a new include
is rejected, reconsider module placement before adding an exception.
