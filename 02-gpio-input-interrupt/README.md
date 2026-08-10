# 02 - GPIO Input Interrupt

## Purpose

Add an external push button on PA0, EXTI0 interrupt capture, a 30 ms thread-mode debounce state machine, and event consumption by Application.

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


Wire a normally-open push button:

```text
PA0 ---- push button ---- GND
```

The GPIO uses the internal pull-up. Therefore:

```text
released -> PA0 HIGH
pressed  -> PA0 LOW
```

The falling transition is routed to EXTI line 0. The onboard PC13 LED remains
the output indicator.


## Compile-Time Configuration


| Setting | Value |
|---|---|
| Button pin | PA0 |
| Input mode | internal pull-up |
| EXTI line | EXTI0 |
| Edge | falling |
| IRQ priority | 2 |
| Debounce time | 30 ms |
| Timebase | 1 ms |
| Status LED | PC13 active-low |

The debounce interval is defined in `config/button_config.h`.


## Initialization Sequence


```text
board_init()
    |
    +--> board_led_init()
    +--> board_timebase_init()
    +--> board_button_init()
            |
            +--> GPIOA + AFIO clocks
            +--> PA0 input pull-up
            +--> GPIO_EXTILineConfig()
            +--> EXTI falling-edge config
            +--> clear pending EXTI0
            +--> NVIC priority + enable

system_init()
    |
    +--> time_service_init()
    +--> indication_service_init()
    +--> button_service_init()
    +--> application_init()
```

`button_service_init()` discards any edge captured during startup.


## Runtime Behavior


The ISR does not debounce:

```text
PA0 falling edge
    |
EXTI0_IRQHandler
    |
    +--> set s_press_edge_pending
    +--> clear EXTI pending bit
    |
return
```

Thread mode performs debounce:

```text
button_service_process()
    |
    +--> take low-level edge?
    |       |
    |       +--> record start time, enable debounce
    |
    +--> 30 ms elapsed?
            |
            +--> no: return
            |
            +--> yes: sample PA0
                       |
                       +--> still pressed -> publish pressed event

application_process()
    |
    +--> take pressed event?
            |
            +--> toggle logical indicator
```

Every new falling edge restarts the debounce window, absorbing mechanical
bounce without blocking the ISR or super-loop.


## SPL / Low-Level Behavior


The board layer uses SPL EXTI/GPIO/RCC APIs and CMSIS NVIC primitives.

The ISR-owned/shared item is the `volatile bool s_press_edge_pending`. Thread
mode clears it using a short PRIMASK-protected critical section so an
interrupt cannot be lost between read and clear.


## Architectural Notes


This example demonstrates the repository's interrupt ownership rule: the BSP
owns EXTI0 because it owns the physical button. The Service owns debounce
policy. The Application owns the decision that a debounced press toggles the
status indication.


## Interrupt and Concurrency Policy

The project follows the repository-wide rule that an interrupt handler belongs
to the lowest module that owns the peripheral. The ISR, when present, may clear
flags, transfer low-level data, and record bounded state. Higher-level policy is
processed later in normal thread mode.

`system_idle()` in this concrete example executes `__NOP()` rather than
`__WFI()`.

## Test Procedure and Expected Result


Flash the firmware, then press and release the PA0 button.

Expected result:

- one deliberate press toggles PC13 once;
- holding the button does not repeatedly toggle;
- contact bounce should not produce multiple toggles;
- a new toggle occurs only after a later press edge.

If one press causes many toggles, verify the button wiring and inspect the
debounce timing. If there is no response, first break at `EXTI0_IRQHandler()`.


## GDB Debugging


```gdb
break EXTI0_IRQHandler
break button_service_process
break indication_service_toggle
continue
```

When stopped in the corresponding source file, inspect:

```gdb
p s_press_edge_pending
p s_debounce_active
p s_debounce_started_ms
p s_pressed_event_pending
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


To move the button to another pin, update the GPIO port/pin, AFIO port source,
EXTI line, IRQ vector, and priority in `board_pins.h`. If the new pin shares an
EXTI group handler such as EXTI9_5, the BSP ISR must be changed accordingly.


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
