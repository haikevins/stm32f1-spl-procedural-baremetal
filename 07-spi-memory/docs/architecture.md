# Architecture — 07-spi-memory

## 1. Dependency Graph

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

Application
    |
Indication Service -> Board LED
Application
    |
Time Service -> Board Timebase
```

## 2. External-Device Driver Composition

Three concerns are deliberately separate:

```text
Application policy
W25Q64 protocol
STM32 SPI wiring
```

The Memory Service is the Application-facing API.

The ECUAL driver owns W25Q64 semantics.

The BSP owns SPI1/CS.

## 3. Ownership Table

| Concern | Owner |
|---|---|
| destructive test policy | Application |
| logical memory operations | Memory Service |
| JEDEC/status/erase/program/read | W25Q64 ECUAL |
| PA4..PA7/SPI1 | Board Memory Bus |
| SPI polling flags | Board Memory Bus |
| LED result | Indication Service/BSP |
| timeout clock | Time Service/Board Timebase |

## 4. Synchronous Transaction Model

Each ECUAL operation returns only after the required SPI transaction and,
where applicable, internal flash BUSY polling are complete.

That makes the API simple but means erase/program can occupy thread mode for a
bounded interval.

## 5. Poll Limits vs Timeouts

This SPL implementation uses millisecond timeouts driven by the board timebase:

```text
SPI transaction: 20 ms
page program: 50 ms
sector erase: 2000 ms
```

These are wall-clock style timeouts, not loop-count limits.

The timebase must therefore be functional before memory operations begin.

## 6. Hardware Transaction Boundary

The board bus owns:

```text
CS assertion
SPI byte transfer
CS deassertion
```

The ECUAL decides which bytes form one device command.

This prevents protocol code from manipulating GPIO directly.

## 7. Read/Write Semantics

Read:

```text
no state change
range validated
```

Program:

```text
1 -> 0 bit programming
Write Enable required
page boundary enforced
BUSY polled
```

Erase:

```text
4 KiB aligned sector
Write Enable required
BUSY polled
```

## 8. Error Propagation

Typical chain:

```text
SPI timeout
    |
board transfer false
    |
W25Q64 operation false
    |
Memory Service false
    |
Application diagnostic/error state
```

Initialization JEDEC failure propagates all the way to `system_panic()`.

## 9. Concurrency

SPI1 is used synchronously by one thread-mode path in this example.

There is no SPI ISR and no shared bus arbitration.

If another device later shares SPI1, explicit ownership/arbitration must be
added.

## 10. Startup Safety

Board initialization:

1. initializes timebase;
2. configures LED;
3. configures SPI/CS;
4. waits memory power-on delay.

Only then does Memory Service query JEDEC ID.

CS is driven HIGH while idle.

## 11. Destructive Boundary

The self-test intentionally owns:

```text
0x007FF000 .. 0x007FFFFF
```

That boundary must be documented and preserved if other firmware data is added.

Do not let unrelated Application data overlap this region.

## 12. Extension Strategy

For a more capable storage subsystem:

```text
Application
    |
Storage Service
    |
record/filesystem layer
    |
Memory Service
    |
W25Q64 ECUAL
    |
Board SPI
```

Keep erase/program geometry below the higher-level record policy.
