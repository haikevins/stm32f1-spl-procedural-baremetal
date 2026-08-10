# 02-gpio-input-interrupt — GPIO Input + EXTI + Debounce Outside the ISR

## 1. Learning Objectives

This example adds an interrupt-driven button without moving product behavior
into the ISR.

You will learn:

- PA0 input with internal pull-up;
- AFIO EXTI routing;
- EXTI0 falling-edge interrupt;
- NVIC priority;
- ISR-to-thread event handoff;
- debounce using timestamps;
- atomic take-and-clear of a shared event flag;
- logical LED control through a Service.

## 2. Wiring

Use a normally-open push button:

```text
PA0 ---- push button ---- GND
```

PA0 uses the STM32 internal pull-up.

Therefore:

```text
released -> HIGH
pressed  -> LOW
```

A press generates a falling edge on EXTI line 0.

The onboard PC13 LED is the output indicator.

## 3. Behavior

Each valid debounced button press toggles the PC13 LED exactly once.

Expected behavior:

- quick contact bounce should not cause multiple toggles;
- holding the button should not repeatedly toggle;
- a later release-and-press produces the next event.

## 4. Configuration

Board mapping:

```text
Button: PA0
EXTI line: 0
IRQ: EXTI0_IRQn
IRQ priority: 2
Polarity: active-low
```

Timebase:

```c
#define BOARD_TIMEBASE_HZ (1000UL)
```

Debounce:

```c
#define BUTTON_DEBOUNCE_TIME_MS (30UL)
```

## 5. Initialization Flow

```text
board_init()
    |
    +--> board_led_init()
    +--> board_timebase_init()
    +--> board_button_init()
            |
            +--> enable GPIOA + AFIO clocks
            +--> configure PA0 input pull-up
            +--> map GPIOA pin 0 to EXTI0
            +--> configure falling edge
            +--> clear pending EXTI
            +--> set NVIC priority
            +--> enable EXTI0 IRQ

system_init()
    |
    +--> time_service_init()
    +--> indication_service_init()
    +--> button_service_init()
    +--> application_init()
```

`button_service_init()` discards a stale edge that may have occurred during
board setup.

## 6. GPIO Input Pull-Up with SPL

The BSP configures PA0 as:

```text
GPIO_Mode_IPU
```

The external switch connects the pin to GND when pressed.

No external pull-up resistor is required for this example.

The logical function:

```c
bool board_button_is_pressed(void);
```

converts the physical level into a boolean pressed state.

## 7. AFIO + EXTI Setup

The BSP:

1. enables AFIO;
2. calls `GPIO_EXTILineConfig()` to route GPIOA pin 0 to EXTI0;
3. configures `EXTI_Mode_Interrupt`;
4. selects `EXTI_Trigger_Falling`;
5. clears stale pending state;
6. enables the NVIC line.

This routing step is required because EXTI line number alone does not encode the
GPIO port.

## 8. ISR Ownership

`EXTI0_IRQHandler()` belongs to the Board Button module.

It performs only:

```text
check EXTI0 pending
    |
set s_press_edge_pending = true
    |
clear EXTI0 pending bit
    |
return
```

It does not debounce and does not call Application.

## 9. Event Handoff from ISR to Thread Mode

The ISR writes:

```c
static volatile bool s_press_edge_pending;
```

Thread mode consumes it through:

```c
bool board_button_take_press_edge(void);
```

The read-and-clear sequence is protected with PRIMASK:

```text
save interrupt state
disable IRQ
read flag
clear flag
restore previous interrupt state
```

This prevents losing an edge between reading and clearing the shared flag.

## 10. Debounce Algorithm

The Button Service owns debounce policy.

When an edge is received:

```text
record current time
set debounce active
```

Every later Service call checks elapsed time.

Before 30 ms:

```text
return immediately
```

After 30 ms:

```text
sample physical button
    |
still pressed?
    |
    +--> yes -> publish one pressed event
    +--> no  -> ignore as bounce/noise
```

A new falling edge restarts the debounce window.

## 11. Application

Application logic is intentionally tiny:

```text
button_service_process()
    |
take debounced pressed event?
    |
    +--> yes -> toggle INDICATION_STATUS
```

Application does not know about PA0, EXTI0, active-low input, or debounce
timing.

## 12. Architecture

```text
Application
    |
    +--> Button Service ------> Board Button -----> EXTI/GPIO SPL
    |
    +--> Indication Service --> Board LED --------> GPIO SPL
    |
    +--> Time Service --------> Board Timebase ----> SysTick
```

## 13. Event Service

This example does not require a generic queue.

The Button Service exposes a one-event pending state because the product
requirement is simply "one debounced press event."

If multiple events must be retained, replace the boolean event with a counter or
queue rather than adding work to the ISR.

## 14. Idle Behavior

The concrete example uses `__NOP()` in `system_idle()`.

The CPU continuously executes the super-loop and immediately processes a button
edge captured by the ISR.

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

## 15. Step-by-Step Test

1. Power the board.
2. Confirm PC13 is initially OFF.
3. Press PA0 button once.
4. Confirm PC13 toggles once.
5. Hold the button; LED should not repeatedly toggle.
6. Release.
7. Press again; LED toggles once again.
8. Set a breakpoint at `EXTI0_IRQHandler()` and verify one or more raw bounce
   edges may occur while only one debounced Application event is produced.

## 16. Troubleshooting

### Press Has No Effect

Check:

- button really connects PA0 to GND;
- PA0 is HIGH when released;
- EXTI0 handler is reached;
- pending bit is cleared;
- Button Service is being processed.

### LED Toggles Multiple Times per Press

Check:

- debounce interval is 30 ms;
- the Service, not the ISR, owns debounce;
- there is no extra Application toggle path;
- switch wiring is not floating.

### EXTI ISR Hits but LED Does Not Change

Then the low-level edge path works.

Inspect:

- `s_press_edge_pending`;
- Button Service debounce state;
- physical button state after 30 ms;
- pending debounced event;
- Indication Service call.

## 17. Extension Exercises

1. Add a released event.
2. Add long-press detection.
3. Add double-click detection.
4. Replace the boolean ISR event with a counter.
5. Move the button to another EXTI line.
6. Add a second button sharing an EXTI group handler.

## 18. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
