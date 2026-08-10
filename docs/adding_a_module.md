# Adding a New Module

## 1. Start from the Requirement, Not from an SPL Function

Describe the behavior first:

```text
"Application needs a debounced press event."
"Application needs a 1 kHz PWM output."
"Application needs to store a record in SPI NOR."
```

Then design the layers.

## 2. Decide Which Layer Owns the Module

| Responsibility | Layer |
|---|---|
| product policy | Application |
| hardware-independent capability | Service |
| physical board resource | BSP |
| off-chip device protocol | ECUAL |
| generic queue/ring/CRC | Common |
| startup composition | System |
| STM32 peripheral operation | SPL/CMSIS below BSP |

## 3. Add SPL Peripheral Support

Identify the SPL peripheral module and the public board-level API required.

### Public Lower-Layer APIs Should Accept Generic Inputs

Prefer:

```c
bool board_bus_init(uint32_t bus_hz);
```

over exposing `SPI_InitTypeDef` to Services.

## 4. Add the SPL Implementation Source

Update `config/modules.mk` with the required vendor `.c` file.

Example:

```make
third_party/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c
```

## 5. Define the Peripheral Configuration Structure

Keep SPL initialization structures inside BSP/lower-layer code.

Examples:

```text
GPIO_InitTypeDef
USART_InitTypeDef
TIM_TimeBaseInitTypeDef
I2C_InitTypeDef
SPI_InitTypeDef
ADC_InitTypeDef
DMA_InitTypeDef
```

Upper layers should never require these types.

## 6. Define Peripheral Constants and Mapping

Add the selected:

- peripheral instance;
- GPIO port;
- pin;
- timer channel;
- DMA channel;
- IRQ;
- active polarity.

Keep these definitions in BSP/configuration rather than Application.

## 7. Add RCC Clock/Reset Support

Enable every required peripheral clock on the correct bus.

Review:

```text
AHB
APB1
APB2
```

If deinitialization/reset is required, perform it before applying the new
configuration.

## 8. Add GPIO Alternate-Function Configuration

Choose the correct electrical mode:

- AF push-pull;
- AF open-drain;
- floating input;
- input pull-up;
- analog input.

Do not copy GPIO mode from an unrelated peripheral.

## 9. Design Polling

Polling should be:

- immediate/non-blocking, or
- bounded by a timeout.

Avoid unbounded loops around SPL flag checks.

## 10. Design Interrupt Handoff

### Event Bit

Use for a simple edge/event.

### Ring Buffer

Use for continuous byte streams.

### Block Event

Use for DMA/sample blocks.

The ISR should publish low-level state and return.

## 11. NVIC

Before enabling the IRQ:

1. initialize peripheral state;
2. clear stale pending flags;
3. set priority;
4. enable the line.

Use CMSIS or SPL `NVIC_Init()` consistently with the project.

## 12. Strong Handler Name

The handler name must exactly match the startup vector.

Examples:

```c
void EXTI0_IRQHandler(void);
void USART1_IRQHandler(void);
void DMA1_Channel1_IRQHandler(void);
```

## 13. Add a Board Resource

Create a BSP API that represents the physical resource.

Examples:

```c
bool board_button_take_press_edge(void);
bool board_uart_try_write_byte(uint8_t byte);
bool board_memory_bus_transfer(...);
```

## 14. `board_pins.h`

Keep the board-specific mapping in the board pin header:

```text
logical resource -> physical pin/peripheral
```

Do not duplicate pin numbers in Service/Application.

## 15. Add an External-Device ECUAL Driver

For off-chip hardware, place command/protocol behavior in ECUAL.

Examples:

```text
SSD1306
W25Q64
sensor
EEPROM
transceiver
```

## 16. Use a Transport/Board-Bus Boundary for ECUAL

Preferred:

```text
ECUAL -> Board Bus -> SPL
```

This keeps the device protocol reusable when pins/peripheral instance change.

## 17. Add a Service

Create a Service when Application needs a stable logical capability.

Examples:

```text
raw edge -> debounced button event
raw ADC samples -> measurement
W25Q64 -> memory API
board LED -> indication API
```

## 18. Service Processing Pattern

Use bounded processing:

```c
void service_process(void)
{
    if (!work_available())
    {
        return;
    }

    /* bounded work */
}
```

Do not make a Service monopolize the super-loop.

## 19. Add Application Behavior

Application should express policy:

```text
if pressed -> toggle indication
if measurement high -> LED on
if period elapsed -> advance state
```

It should not express SPL/peripheral operations.

## 20. Update `system_init()`

Initialize dependencies from bottom to top:

```text
board
services
external device
application
```

Return failure if a required initialization step fails.

## 21. Global IRQ Lifecycle

Know when interrupts can run.

Do not assume an interrupt-driven timebase is advancing during a section where
interrupts are globally disabled.

Keep initialization sequences compatible with the actual IRQ lifecycle.

## 22. Add Configuration

Put tunable values in `config/`:

```text
baud
frequency
timeout
buffer size
address
sample rate
threshold
debounce time
```

## 23. Compile-Time Validation

Reject impossible configurations early.

Examples:

```text
ring size < 2
DMA buffer odd
PWM period outside 16-bit range
invalid page length
zero timeout/frequency
```

Use `#if` or `_Static_assert` as appropriate.

## 24. Do Not Use Heap Allocation

Prefer static buffers with explicit capacity.

This keeps SRAM use visible and makes ISR ownership easier to reason about.

## 25. Add Debug Observability

Expose meaningful state when useful:

```text
overflow count
error count
JEDEC ID
measurement sequence
verification status
```

GDB observability is often enough without adding a debug UART dependency.

## 26. Update Documentation

### README

Document wiring, configuration, behavior, test steps, and troubleshooting.

### `architecture.md`

Document ownership, initialization, ISR handoff, and concurrency.

### `porting_guide.md`

Document pin, clock, peripheral, IRQ, and validation changes.

## 27. Run the Layer Checker

```bash
make check-layers
```

Fix the design if a forbidden include appears.

## 28. Build Cleanly

```bash
make clean
make
```

## 29. Inspect Map/Symbols

Use:

```bash
make size
```

and inspect map/listing symbols when memory ownership or interrupt-handler
binding is uncertain.

## 30. Hardware Bring-Up Strategy

Bring the system up from bottom to top:

1. power and wiring;
2. GPIO;
3. peripheral clock;
4. peripheral flags/data;
5. IRQ/DMA;
6. Service;
7. Application.

## 31. Module Addition Checklist

-  requirement defined;
-  correct layer selected;
-  SPL source added;
-  BSP mapping added;
-  peripheral clock verified;
-  GPIO electrical mode correct;
-  polling bounded;
-  IRQ ownership correct;
-  shared state safe;
-  Service API hardware-independent;
-  Application has no lower-layer include;
-  debug state available;
-  README updated;
-  architecture updated;
-  porting guide updated;
-  layer checker passes;
-  clean build succeeds;
-  hardware test passes.
