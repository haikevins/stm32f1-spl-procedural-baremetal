# Porting Guide — 07-spi-memory

## 1. Changing SPI/CS Pins on STM32F103

Update Board Memory Bus mappings:

- SCK;
- MISO;
- MOSI;
- CS;
- GPIO clocks;
- remap if required.

Keep W25Q64 ECUAL unchanged.

## 2. Moving to SPI2

Review:

- SPI2 APB bus;
- GPIO mapping;
- RCC clock;
- prescaler input clock;
- SPL peripheral instance.

The Memory Service and W25Q64 driver should remain unchanged.

## 3. Changing SPI Frequency

Update:

```c
BOARD_MEMORY_SPI_MAX_HZ
```

The BSP selects a prescaler that does not exceed the configured maximum.

Verify actual SCK with a logic analyzer.

## 4. Using a Different Flash Capacity

Review together:

- total size;
- JEDEC capacity code;
- address width;
- page size;
- sector size;
- erase commands;
- test sector.

Do not change only the capacity ID.

## 5. Changing the Test Sector

Choose an aligned 4 KiB sector inside the device.

Document that the region is destructive.

Verify it does not overlap boot/config/user data.

## 6. Multi-Page Programming

The current Page Program function refuses to cross one 256-byte page.

A higher-level multi-page helper should split a buffer:

```text
remaining bytes
    |
current page free space
    |
program chunk
    |
advance address
```

Do not remove the page-boundary check.

## 7. Runtime Asynchronous Operation

If long erase/program latency becomes unacceptable, convert the Memory Service
to a state machine:

```text
start operation
poll status in service_process()
publish completion event
```

Keep SPI/device ownership below Application.

## 8. Shared SPI Bus

Add a bus-ownership mechanism if another SPI device shares SCK/MISO/MOSI.

Each device keeps a separate CS.

Bus configuration must remain compatible or be reconfigured safely per device.

## 9. Porting to Another MCU

Keep:

```text
Memory Service -> W25Q64 ECUAL
```

Replace Board Memory Bus and vendor SPI implementation.

## 10. Validation Checklist

- [ ] CS idle HIGH;
- [ ] SPI mode 0 correct;
- [ ] SCK below configured maximum;
- [ ] JEDEC ID correct;
- [ ] WEL sets after Write Enable;
- [ ] BUSY clears after program/erase;
- [ ] read-back matches;
- [ ] test sector is safe/destructive by design;
- [ ] layer checker passes.

## 11. Common Pitfalls

- D0/D1 reversed;
- using 5 V;
- CS toggled between command and address/data;
- wrong SPI mode;
- forgetting dummy clocks on reads;
- crossing page boundary;
- not issuing Write Enable;
- using an unbounded BUSY loop;
- erasing data outside the documented test region.
