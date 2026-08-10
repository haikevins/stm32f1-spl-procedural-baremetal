# 07 - SPI W25Q64 Memory

## Purpose

Communicate with a W25Q64-compatible 64-Mbit SPI NOR device, read JEDEC ID, erase a sector, program one page fragment, read it back, and verify data.

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


Wire the six-pin module:

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

Use 3.3 V supply and logic.


## Compile-Time Configuration


| Setting | Value |
|---|---|
| SPI | SPI1 |
| CS | PA4 |
| SCK | PA5 |
| MISO | PA6 |
| MOSI | PA7 |
| SPI mode | 0 |
| Maximum requested SPI clock | 5 MHz |
| SPI polling timeout | 20 ms |
| Power-on delay | 10 ms |
| Page-program timeout | 50 ms |
| Sector-erase timeout | 2000 ms |
| Test sector | 0x007FF000 |
| Test length | 32 bytes |
| Heartbeat | 500 ms |

With the normal 72 MHz PCLK2, the BSP selects `/16`, producing 4.5 MHz.


## Initialization Sequence


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


## Runtime Behavior


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


## SPL / Low-Level Behavior


The SPI BSP is polling. For every byte it:

1. waits for TXE;
2. writes the transmit byte or `0xFF` dummy byte;
3. waits for RXNE;
4. reads the received byte;
5. after the transfer, waits for BSY to clear.

Chip select is controlled explicitly by GPIO around each command transaction.

The ECUAL validates address range and rejects Page Program operations that
cross a 256-byte page boundary.


## Architectural Notes


The Memory Service hides the concrete W25Q64 device from Application. The ECUAL
owns NOR command semantics. The BSP owns SPI1 and CS pin transactions. This
keeps board wiring and flash protocol concerns separate.


## Interrupt and Concurrency Policy

The project follows the repository-wide rule that an interrupt handler belongs
to the lowest module that owns the peripheral. The ISR, when present, may clear
flags, transfer low-level data, and record bounded state. Higher-level policy is
processed later in normal thread mode.

`system_idle()` in this concrete example executes `__NOP()` rather than
`__WFI()`.

## Test Procedure and Expected Result


**Warning:** every reset erases the last 4 KiB sector. Do not store important
data there while running this demo.

After flashing:

- valid JEDEC identification + passing erase/program/read-back test -> PC13
  toggles every 500 ms;
- erase/program/read-back/verify failure after successful device
  identification -> PC13 stays ON;
- JEDEC/device initialization failure -> `system_init()` fails and the firmware
  enters `system_panic()`, so use GDB to diagnose that earlier failure.

The most useful first debug check is JEDEC ID. For a common W25Q64 module the
observed bytes are typically `EF 40 17`; the source formally validates `EF` and
capacity `17`.


## GDB Debugging


Application exports diagnostics intentionally:

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

Expected successful test address:

```text
0x007FF000
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


To use another SPI instance, update SCK/MISO/MOSI/CS mapping and the APB clock
used for prescaler selection. To support another NOR geometry, review size,
page size, sector size, JEDEC validation, address width, command set, and
timeouts together.


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
