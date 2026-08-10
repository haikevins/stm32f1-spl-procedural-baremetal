# 07-spi-memory — SPI1 + W25Q64 NOR Flash

## 1. Learning Objectives

This example demonstrates a synchronous external NOR flash driver.

You will learn:

- SPI1 mode 0;
- software-controlled chip select;
- SPI polling with bounded waits;
- JEDEC ID read;
- Status Register-1 BUSY/WEL bits;
- Write Enable;
- 4 KiB sector erase;
- 256-byte page-program rules;
- read-back verification;
- destructive test boundaries;
- separation of memory protocol from board SPI wiring.

## 2. Wiring

```text
STM32F103C8T6       W25Q64
--------------------------------
3.3V        ------  VCC
GND         ------  GND
PA4         ------  CS
PA5         ------  CLK
PA6         ------  D1 / DO / MISO
PA7         ------  D0 / DI / MOSI
```

Use 3.3 V only.

## 3. Expected Behavior

At startup the firmware:

1. waits for the memory to be ready;
2. reads JEDEC ID;
3. verifies Winbond manufacturer and 64-Mbit capacity code;
4. erases the last 4 KiB sector;
5. programs a 32-byte test pattern;
6. reads it back;
7. compares every byte.

If the full self-test passes:

```text
PC13 toggles every 500 ms
```

If erase/program/read-back verification fails after successful memory
initialization:

```text
PC13 stays ON
```

If JEDEC/device initialization itself fails, `system_init()` fails and the
firmware enters `system_panic()`.

## 4. W25Q64 Geometry in the Driver

```text
total size: 8 MiB = 8,388,608 bytes
address range: 0x000000 .. 0x7FFFFF
page size: 256 bytes
sector size: 4096 bytes
```

The demo uses:

```text
last sector start = 0x007FF000
```

## 5. JEDEC ID

Command:

```text
0x9F
```

Three bytes are read:

```text
manufacturer
memory type
capacity
```

The driver requires:

```text
manufacturer = 0xEF
capacity     = 0x17
```

The memory-type byte is recorded but not restricted to one exact value so the
driver remains useful across compatible W25Q64 revisions.

A common observed result is:

```text
EF 40 17
```

## 6. SPI Configuration

```text
peripheral: SPI1
mode: master
direction: 2-line full duplex
data size: 8 bit
CPOL: 0
CPHA: first edge
first bit: MSB
NSS: software
```

That is SPI mode 0.

CS is controlled separately on PA4.

## 7. SPI Clock Selection

The configured maximum is:

```c
#define BOARD_MEMORY_SPI_MAX_HZ (5000000UL)
```

The BSP reads PCLK2 and selects the fastest SPL baud prescaler that does not
exceed the requested maximum.

With PCLK2 = 72 MHz:

```text
/16 -> 4.5 MHz
```

## 8. Chip Select

A complete flash command transaction is wrapped by:

```text
CS LOW
    |
command/address/data clocks
    |
CS HIGH
```

CS stays low across multi-part operations such as:

```text
command header
then receive payload
```

The BSP drives CS high during initialization so the flash remains deselected.

## 9. SPI Transfer Primitive

For each byte:

```text
wait TXE
write byte
wait RXNE
read byte
```

If the caller wants receive-only behavior, the BSP sends dummy `0xFF` bytes.

After the transfer it waits for BSY to clear before returning.

All waits are bounded by the board timebase timeout.

## 10. W25Q64 Command Set Used by the Demo

| Command | Value |
|---|---:|
| Write Enable | `0x06` |
| Read Status Register-1 | `0x05` |
| Read Data | `0x03` |
| Page Program | `0x02` |
| 4 KiB Sector Erase | `0x20` |
| JEDEC ID | `0x9F` |

## 11. Status Register-1

### BUSY

Bit 0 indicates an internal program/erase operation is still active.

The driver polls until BUSY clears or a timeout expires.

### WEL

Bit 1 is Write Enable Latch.

Before erase/program:

```text
send 0x06
    |
read status
    |
WEL set?
```

If WEL is not set, the write operation is not started.

## 12. Bounded Polling

Internal flash operations can take much longer than one SPI byte.

Configured limits:

```text
SPI transaction timeout: 20 ms
page program timeout: 50 ms
sector erase timeout: 2000 ms
```

The driver repeatedly reads Status Register-1 and compares elapsed milliseconds.

No infinite BUSY loop is used.

## 13. Page Program Rules

A Page Program:

- requires Write Enable;
- may program at most 256 bytes;
- must not cross a 256-byte page boundary in this driver;
- is followed by BUSY polling.

The driver checks:

```text
page_offset + length <= 256
```

before issuing the command.

## 14. Sector Erase Rules

The driver aligns the supplied address down to a 4 KiB boundary:

```text
address -= address % 4096
```

Then:

```text
wait ready
write enable
send 0x20 + 24-bit address
wait BUSY clear
```

## 15. Read Transaction

```text
CS LOW
0x03
A23..A16
A15..A8
A7..A0
dummy clocks -> data bytes
CS HIGH
```

The driver validates the requested address range before beginning.

## 16. ECUAL Transport

```text
Application
    |
Memory Service
    |
W25Q64 ECUAL
    |
Board Memory Bus
    |
SPI1/GPIO/RCC SPL
```

The W25Q64 driver owns command/geometry semantics.

The board bus owns SPI1 pins and chip select.

## 17. Application Self-Test

Pattern length:

```text
32 bytes
```

Test address:

```text
0x007FF000
```

Sequence:

```text
erase
program
read
verify
```

The first mismatch index is recorded if verification fails.

## 18. Debug Globals

The Application deliberately exports bring-up state:

```gdb
p/x application_memory_manufacturer_id
p/x application_memory_type_id
p/x application_memory_capacity_id

p/x application_memory_test_address
p application_memory_erase_ok
p application_memory_program_ok
p application_memory_verify_ok
p application_memory_test_passed
p application_memory_error_count
p/x application_memory_first_mismatch_index
p/x application_memory_readback_first_byte
p/x application_memory_readback_last_byte
```

## 19. LED Indication

After a successful self-test:

```text
PC13 toggles every 500 ms
```

After a later self-test failure path handled by Application:

```text
PC13 steady ON
```

JEDEC initialization failure is earlier and enters panic.

## 20. Interrupt Policy

SPI interrupts are not used.

SPI transactions are synchronous polling operations.

SysTick provides timeout/heartbeat milliseconds.

## 21. Architecture

```text
Application
    |
Memory Service
    |
W25Q64 ECUAL
    |
Board Memory Bus
    |
SPI1/GPIO SPL
```

Indication and Time Services are separate logical dependencies.

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

## 22. Test Procedure

**Warning:** the last 4 KiB sector is erased on every reset.

1. Wire the W25Q64.
2. Flash firmware.
3. Inspect JEDEC globals.
4. Verify manufacturer/capacity.
5. Verify erase/program/verify booleans.
6. Verify error count is zero.
7. Confirm heartbeat LED.
8. Optionally capture SPI with a logic analyzer.

## 23. JEDEC Troubleshooting

### `00 00 00`

Likely causes:

- MISO stuck low;
- wrong D1/DO wiring;
- no power;
- no common ground.

### `FF FF FF`

Likely causes:

- MISO floating/high;
- CS never asserted;
- device not selected;
- wiring open.

### ID Is Correct but Program/Erase Fails

Basic SPI wiring is mostly proven.

Focus on:

- Write Enable;
- WEL status;
- BUSY polling;
- erase address;
- page boundary;
- timeout.

## 24. Logic Analyzer

Expected JEDEC transaction:

```text
CS low
9F
FF -> EF
FF -> type
FF -> 17
CS high
```

Expected command mode:

```text
CPOL = 0
sample on first edge
```

Check CS remains low for the entire command/address/data phase.

## 25. Wear/Endurance Note

This demo erases the same sector at every reset.

That is convenient for education but not a production data-management
strategy.

Do not reset continuously for no reason and do not store important data in the
test sector.

A real application should implement allocation, wear policy, or a filesystem as
required.

## 26. Extension Exercises

1. Add multi-page programming.
2. Add block/chip erase.
3. Add device-capacity detection.
4. Add a shared SPI bus abstraction.
5. Add asynchronous state-machine-based erase/program.
6. Build a simple key-value store.
7. Add CRC to stored records.

## 27. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
