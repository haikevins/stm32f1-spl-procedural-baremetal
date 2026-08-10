# 01 - Blink LED

## Purpose

Introduce GPIO output, a 1 ms SysTick timebase, layered board/services APIs, and a non-blocking periodic Application task.

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


The example uses the Blue Pill onboard LED connected to PC13. The board layer
defines it as active-low:

```text
PC13 LOW  -> LED ON
PC13 HIGH -> LED OFF
```

No external components are required.


## Compile-Time Configuration


| Setting | Source | Value |
|---|---|---|
| Status LED | `board_pins.h` | PC13, active-low |
| Timebase | `board_config.h` | 1000 Hz |
| Application toggle period | `application_config.h` | 500 ms |

`board_led_init()` presets the inactive output level before configuring the pin
as push-pull output, then `indication_service_init()` explicitly requests the
logical OFF state.


## Initialization Sequence


```text
SystemInit()
    |
main()
    |
system_init()
    |
    +--> board_init()
    |      +--> SystemCoreClockUpdate()
    |      +--> board_led_init()
    |      +--> board_timebase_init()
    |
    +--> time_service_init()
    +--> indication_service_init()
    +--> application_init()
```

`board_timebase_init()` configures SysTick from `SystemCoreClock /
BOARD_TIMEBASE_HZ`. `SysTick_Handler()` increments a millisecond counter.


## Runtime Behavior


Application stores the last toggle timestamp. Every call to
`application_process()` asks `time_service_periodic_due()` whether 500 ms has
elapsed.

```text
super-loop
    |
    +--> application_process()
            |
            +--> time_service_periodic_due()
                    |
              false +--> return immediately
                    |
               true +--> indication_service_toggle()
```

No delay loop is used to create the blink period.


## SPL / Low-Level Behavior


SPL usage is intentionally small:

- `RCC_APB2PeriphClockCmd()` enables GPIOC.
- `GPIO_Init()` configures PC13 as 2 MHz push-pull output.
- `SysTick_Config()` creates the 1 kHz timebase.
- `NVIC_SetPriority()` places SysTick at the lowest implemented priority.

The only active interrupt is SysTick.


## Architectural Notes


The main architectural lesson is that the Application does not know that the
indicator is PC13 or active-low. It only asks for the logical status indicator
to toggle. This makes a later change to a different LED or output device a BSP
change rather than an Application rewrite.


## Interrupt and Concurrency Policy

The project follows the repository-wide rule that an interrupt handler belongs
to the lowest module that owns the peripheral. The ISR, when present, may clear
flags, transfer low-level data, and record bounded state. Higher-level policy is
processed later in normal thread mode.

`system_idle()` in this concrete example executes `__NOP()` rather than
`__WFI()`.

## Test Procedure and Expected Result


After flashing, the onboard LED should change state every 500 ms.

One full ON/OFF cycle therefore takes about 1 second.

If the LED never changes:

1. verify the board really uses PC13 for the onboard LED;
2. break in `SysTick_Handler()` and confirm interrupts occur;
3. inspect `s_time_ms` in `board_timebase.c`;
4. break in `application_process()` and confirm the super-loop runs;
5. inspect the GPIO output state.


## GDB Debugging


Useful GDB commands:

```gdb
break SysTick_Handler
break application_process
break indication_service_toggle
continue
```

File-static variables are built with debug information, so they can also be
inspected when stopped in the relevant source context:

```gdb
p s_time_ms
p s_last_toggle_ms
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


To move the LED, change `bsp/bluepill/src/board_pins.h` and, if necessary, the
GPIO clock. To change the timebase frequency, update `BOARD_TIMEBASE_HZ`, but
keep the Service's millisecond semantics consistent or update the Service API
accordingly.


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
