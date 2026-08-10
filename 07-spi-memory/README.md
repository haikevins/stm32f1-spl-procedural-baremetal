# 07 - SPI Memory: W25Q64

This example replaces the previous SPI sensor example with a Winbond W25Q64
Serial NOR Flash demo using STM32F103 SPI1 and the Standard Peripheral Library.

## Wiring

For the common six-pin W25Q64 module labeled `D1 CLK GND D0 CS VCC`:

| Blue Pill | W25Q64 module | Meaning |
|---|---|---|
| 3.3V | VCC | 3.3 V supply |
| GND | GND | common ground |
| PA5 | CLK | SPI1 SCK |
| PA6 | D1 | flash DO / IO1 -> MCU MISO |
| PA7 | D0 | flash DI / IO0 <- MCU MOSI |
| PA4 | CS | active-low chip select |

If your board silkscreen appears to say `VCO`, verify it carefully; the
Winbond flash device supply pin is named `VCC`.

Do not power the flash or SPI signals from 5 V.

## SPI configuration

- SPI1
- mode 0 (CPOL=0, CPHA=0)
- MSB first
- 8-bit full duplex
- software chip select on PA4
- configured maximum: 5 MHz
- normal 72 MHz PCLK2 results in 4.5 MHz SPI

## Driver operations

The ECUAL driver implements:

- `0x9F` - JEDEC ID
- `0x05` - Status Register-1
- `0x06` - Write Enable
- `0x03` - Read Data
- `0x02` - Page Program
- `0x20` - 4 KiB Sector Erase

The driver checks WEL after Write Enable and polls BUSY after page program or
sector erase. A page-program request is rejected if it crosses a 256-byte
page boundary.

W25Q64 uses an 8 MiB address space and 24-bit addresses.

## Destructive demo

**Warning:** every reset erases the final 4 KiB sector:

```text
0x007FF000 .. 0x007FFFFF
```

The example then programs 32 bytes at `0x007FF000`, reads them back, and
compares every byte.

This leaves the beginning of the flash untouched, but the final sector must be
considered reserved for this example.

## LED behavior

- successful erase/program/verify: PC13 toggles every 500 ms
- test failure after a valid JEDEC initialization: PC13 stays ON
- board or JEDEC initialization failure: firmware enters `system_panic()`

`system_idle()` remains `__NOP()` to preserve reliable SWD re-attachment with
ST-Link adapters that do not expose NRST.

## GDB variables

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

p/x application_memory_readback_first_byte
p/x application_memory_readback_last_byte
p/x application_memory_first_mismatch_index
```

For a Winbond W25Q64, the manufacturer byte should be `0xEF`. The example also
expects the 64-Mbit capacity code used by this device family.

## Architecture

```text
Application
    |
    v
Memory Service
    |
    v
W25Q64 ECUAL
    |
    v
Board Memory Bus
    |
    v
GPIO / SPI1
```

## Build

```bash
make check-layers
make clean
make
make flash
```
