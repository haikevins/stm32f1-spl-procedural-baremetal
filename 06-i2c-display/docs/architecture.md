# Architecture — 06-i2c-display

## 1. Dependency Graph

```text
Application
    |
Display Service
    |
SSD1306 ECUAL
    |
Board Display Bus
    |
I2C1/GPIO/RCC SPL

Application
    |
Time Service
    |
Board Timebase
    |
SysTick
```

## 2. Why the Service Is the Application-Facing Composition Point

Application should depend on a display capability, not on SSD1306 command
details.

The Service exposes operations such as:

```text
clear
draw text
draw progress bar
present
```

The concrete external device remains below that boundary.

## 3. Ownership Table

| Concern | Owner |
|---|---|
| screen content | Application |
| display capability | Display Service |
| glyph/framebuffer/controller | SSD1306 ECUAL |
| I2C address transaction | Board Display Bus |
| PB6/PB7/I2C1 | BSP |
| I2C registers/functions | SPL |
| refresh schedule | Application + Time Service |

## 4. Initialization Timing Nuance

The timebase must exist before SSD1306 initialization because:

- the module needs a power-on delay;
- I2C polling uses bounded millisecond timeouts.

Therefore board initialization starts the timebase before the display bus.

## 5. I2C Polling Semantics

I2C is synchronous from the caller's perspective.

Each transaction either:

- completes;
- fails on an I2C error;
- fails on timeout.

No I2C ISR can asynchronously call upward.

## 6. Framebuffer Ownership

The 1024-byte framebuffer belongs to the SSD1306 ECUAL.

Application never writes framebuffer bytes directly.

This allows future changes in page layout or controller mapping below the
Service API.

## 7. Runtime Update Flow

```text
100 ms elapsed
    |
Application advances progress
    |
clear framebuffer
    |
draw text/progress
    |
Display Service present
    |
SSD1306 set address window
    |
Board I2C write framebuffer
```

## 8. Failure Model

Initialization failure propagates to `system_init()` and panic.

Runtime present failure changes Application's display-operational state to
false, preventing repeated failing transfers.

A production design may add retry/recovery policy in a Service.

## 9. Layer Boundary Benefits

The same Application could theoretically render through:

- another OLED controller;
- SPI display;
- simulated host display;

as long as the Display Service contract is preserved.

The SSD1306 driver itself can also be reused with another board I2C
implementation.

## 10. Extension Notes

Good extensions:

- partial update in ECUAL;
- richer graphics in Display Service/ECUAL;
- bus recovery in BSP;
- application screens in Application.

Avoid embedding PB6/PB7 or I2C flags in drawing code.
