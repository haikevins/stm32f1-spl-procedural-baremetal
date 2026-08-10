# Template Architecture and Dependency Rules

## 1. Runtime Layers

```text
Application
    |
    v
Services
    |
    +------> BSP
    |
    +------> ECUAL
                  |
                  v
          STM32F10x SPL
                  |
                  v
               CMSIS
                  |
                  v
            STM32F103
```

`system/` is the composition root rather than a dependency layer consumed by
Application.

## 2. Dependency Matrix

| Source layer | Allowed project dependencies |
|---|---|
| Application | Application, Services, Common, Config |
| Services | Services, BSP, ECUAL, Common, Config |
| ECUAL | ECUAL, BSP, Common, Config |
| BSP | BSP, Common, Vendor, Config |
| Common | Common, Config |
| Platform | Platform, Common, Vendor, Config |
| Runtime | Runtime, Common, Vendor, Config |
| System | All runtime layers + Vendor + Config |

The layer checker classifies STM32F10x SPL and CMSIS headers as Vendor.

## 3. Application

Application owns product/demo policy and state machines.

### Good

```text
if button event -> toggle logical indicator
if 500 ms elapsed -> advance state
if voltage > threshold -> set logical output
```

### Bad

```text
GPIO_ResetBits(GPIOC, GPIO_Pin_13)
USART_SendData(USART1, byte)
DMA_Cmd(DMA1_Channel1, ENABLE)
```

Those are lower-layer implementation details.

## 4. Services

Services expose hardware-independent capabilities such as:

- time;
- indication;
- debounced button events;
- UART bytes;
- PWM duty;
- display operations;
- memory operations;
- ADC measurements.

Services may filter, debounce, aggregate, or translate units, but must not
become a second BSP.

## 5. BSP

BSP owns Blue Pill hardware mapping:

- GPIO port/pin;
- active polarity;
- peripheral instance;
- RCC clock;
- timer channel;
- IRQ line and priority;
- low-level interrupt handler for owned resources.

BSP may call SPL/CMSIS directly.

## 6. ECUAL

ECUAL owns protocols for off-chip devices such as SSD1306 and W25Q64.

### Transport Callback Pattern

In this SPL repository the external-device driver normally calls a board-bus
API:

```text
ECUAL
  |
  v
Board Bus
  |
  v
SPL peripheral
```

This prevents SSD1306/W25Q64 code from depending on a particular STM32 pin map.

## 7. SPL Peripheral Layer

STM32F10x SPL acts as the MCAL-equivalent peripheral implementation.

It provides structured configuration and operations for:

```text
RCC
GPIO
EXTI
USART
TIM
I2C
SPI
ADC
DMA
```

Only lower layers should include these headers.

## 8. CMSIS Device Layer

CMSIS device headers provide STM32F103 peripheral register definitions,
interrupt names, and device-level constants used by SPL and occasional BSP
code.

The project defines the medium-density device class with `STM32F10X_MD`.

## 9. CMSIS Architecture Layer

CMSIS Cortex-M3 support owns architecture-level operations such as:

- `__NOP()`;
- `__WFI()`;
- `__disable_irq()`;
- `__enable_irq()`;
- PRIMASK access;
- NVIC operations;
- `SysTick_Config()`.

These facilities belong in System/BSP/platform code rather than Application.

## 10. Common

Common code is hardware-independent.

Example 04's byte ring buffer belongs here because it knows only storage,
head/tail indices, and a producer/consumer contract.

## 11. System as Composition Root

System may include all public layer APIs because it wires modules together.

Typical order:

```text
board_init()
service_init()
external_device_init()
application_init()
```

System must not contain product behavior that belongs in Application.

## 12. Initialization Order

Initialize from physical dependencies upward:

```text
clock/pin/peripheral
        |
        v
board resource
        |
        v
external device/service
        |
        v
application
```

Examples:

- Time Service requires Board Timebase first.
- SSD1306 initialization requires I2C bus and timebase first.
- Application memory self-test requires Memory Service/W25Q64 first.

## 13. Interrupt Ownership

The lowest module that owns the interrupt source implements the strong handler.

Examples:

```text
Board Timebase -> SysTick_Handler
Board Button   -> EXTI0_IRQHandler
Board UART     -> USART1_IRQHandler
Board ADC/DMA  -> DMA1_Channel1_IRQHandler
```

An ISR must not call Application.

## 14. Interrupt-to-Thread Handoff Patterns

### Event Bit

Useful for a simple edge:

```text
ISR: pending = true
thread: take + clear
```

### Counter

Useful when event count or diagnostic count matters.

### Ring Buffer

Useful for byte streams:

```text
RX ISR producer -> ring -> thread consumer
thread producer -> ring -> TX ISR consumer
```

### Block-Ready

Useful for DMA:

```text
DMA ISR -> stable completed block -> thread Service
```

## 15. Critical Sections

Use short critical sections only when a multi-step shared-state operation must
be atomic.

Correct:

```text
save PRIMASK
disable interrupts
copy/read-clear shared state
restore PRIMASK
```

Incorrect:

```text
disable interrupts
perform I2C/SPI transaction
format display
restore interrupts
```

## 16. `volatile`

Use `volatile` for state changed asynchronously by hardware/ISR context.

`volatile` does not provide:

- mutual exclusion;
- atomic multi-step operations;
- queue correctness;
- memory ownership.

Those require explicit design.

## 17. Polling API Naming

Prefer API names that describe immediate/non-blocking semantics:

```text
try_read
try_write
can_read
can_write
take_event
process
```

For synchronous operations that may wait, document and enforce a timeout.

## 18. Error Handling

Convert low-level failures into bounded state:

- `bool` failure;
- status value;
- error counter;
- overflow counter.

Examples include UART RX overflow, I2C transaction failure, W25Q64 timeout,
DMA transfer error, and ADC calibration failure.

## 19. Clock Ownership

Application should use human-level units:

```text
milliseconds
baud
Hz
permille
millivolts
```

BSP/peripheral code converts those requests using the actual clock tree.

Do not make Application depend on PCLK1/PCLK2 or timer prescalers.

## 20. Board Active Level

Logical Services hide electrical polarity.

For the active-low PC13 LED:

```text
indication_service_set(true)
```

still means "turn the indicator on."

The BSP decides that the physical GPIO level is LOW.

## 21. External-Device Geometry

External-device geometry belongs below Application.

Examples:

```text
SSD1306: 128 x 64, 1024-byte framebuffer
W25Q64: 8 MiB, 256-byte page, 4 KiB sector
```

Application should not build SPI/I2C command frames.

## 22. Build-Time Configuration

Use `config/` for project constants:

- baud rate;
- debounce interval;
- buffer size;
- timer frequency;
- I2C/SPI speed;
- device address;
- timeout;
- sample rate;
- thresholds.

Use compile-time checks for invalid relationships where practical.

## 23. Dependency Checker Limitations

The checker validates include direction.

It cannot detect:

- race conditions;
- hidden coupling through globals;
- long ISR latency;
- incorrect clock configuration;
- electrical wiring errors;
- misuse of a correct SPL API.

Architecture review and hardware testing are still required.

## 24. Architectural Acceptance Checklist

-  Application has no BSP/SPL/CMSIS includes.
-  Services contain no raw peripheral calls.
-  BSP owns physical board mapping.
-  ECUAL owns external-device protocol.
-  SPL/CMSIS remain below upper layers.
-  ISR belongs to the lowest owner.
-  ISR work is bounded.
-  thread handoff is explicit.
-  shared state has clear ownership.
-  clock conversions occur below Application.
-  errors/overflows are observable.
-  `make check-layers` passes.
