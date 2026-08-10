# 01-blink-led — GPIO Output + SysTick + Non-Blocking Super-Loop

## 1. Learning Objectives

This example introduces the core architecture used by the entire repository.

You will learn how to:

- configure the Blue Pill onboard LED through the BSP;
- represent an active-low output as a logical indication;
- create a 1 ms SysTick timebase;
- schedule periodic work without a blocking delay;
- keep Application independent from GPIO/SPL details;
- trace startup from `Reset_Handler` to the super-loop.

## 2. Expected Result

After flashing, the onboard PC13 LED changes state every 500 ms.

Because the LED is active-low:

```text
PC13 LOW  -> LED ON
PC13 HIGH -> LED OFF
```

One complete ON/OFF cycle therefore takes approximately one second.

## 3. Hardware

No external components are required.

The example uses the onboard Blue Pill LED:

```text
PC13 -> onboard LED
```

The board abstraction defines this resource as `BOARD_STATUS_LED_ACTIVE_LOW`.

## 4. Compile-Time Configuration

`config/board_config.h`:

```c
#define BOARD_HSE_FREQUENCY_HZ (8000000UL)
#define BOARD_TIMEBASE_HZ       (1000UL)
```

`config/application_config.h`:

```c
#define APPLICATION_BLINK_PERIOD_MS (500UL)
```

The Application works in milliseconds and does not know the SysTick reload
value or CPU frequency.

## 5. Startup Flow

```text
Reset_Handler
    |
    +--> initialize .data and .bss
    +--> SystemInit()
    +--> main()
            |
            +--> system_init()
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

After initialization:

```text
for (;;)
{
    application_process();
    system_idle();
}
```

## 6. Clock Setup

This SPL project relies on the CMSIS `SystemInit()` implementation for the
system clock configuration.

`board_init()` calls:

```c
SystemCoreClockUpdate();
```

before clock-dependent board modules are initialized.

The timebase uses:

```c
SysTick_Config(SystemCoreClock / BOARD_TIMEBASE_HZ)
```

With a 1 kHz timebase, SysTick produces one interrupt every millisecond.

The Application never uses `SystemCoreClock` directly.

## 7. GPIO LED

`board_led_init()`:

1. enables the GPIOC peripheral clock with SPL;
2. presets the inactive LED level;
3. configures PC13 as a 2 MHz push-pull output.

The logical API hides active-low behavior:

```c
board_led_set(true);   /* logical ON */
board_led_set(false);  /* logical OFF */
```

`indication_service_set()` and `indication_service_toggle()` therefore remain
independent from the electrical polarity.

## 8. SysTick Timebase

The BSP owns the physical timebase.

`SysTick_Handler()` performs only:

```c
s_time_ms++;
```

The Time Service exposes millisecond semantics:

```c
uint32_t time_service_get_ms(void);
uint32_t time_service_elapsed_ms(uint32_t start_time_ms);
bool time_service_periodic_due(uint32_t *last_run_ms,
                               uint32_t period_ms);
```

The Application does not access SysTick directly.

## 9. Application State

The Application stores one timestamp:

```c
static uint32_t s_last_toggle_ms;
```

Every loop:

```text
500 ms elapsed?
    |
    +--> no  -> return
    |
    +--> yes -> toggle INDICATION_STATUS
```

There is no software delay loop.

This allows the same super-loop to later process UART, buttons, display, or
other Services without being blocked for 500 ms.

## 10. Architecture

```text
Application
    |
    +--> Time Service ------> Board Timebase ------> SysTick/CMSIS
    |
    +--> Indication Service -> Board LED ----------> GPIO/SPL
```

Application depends only on logical Services.

See [`docs/architecture.md`](docs/architecture.md) for ownership details.

## 11. Interrupt

The only active interrupt is SysTick.

Ownership:

```text
SysTick hardware
    |
    v
Board Timebase
    |
    v
SysTick_Handler()
```

The ISR only increments the time counter. It does not call the Time Service or
Application.

## 12. Idle and Panic

This completed example uses:

```c
void system_idle(void)
{
    __NOP();
}
```

Panic disables interrupts and stays in a `__NOP()` loop.

The choice is intentionally debug-friendly for ST-Link setups without NRST.

## 13. Recommended Reading Order

Read:

1. `app/src/application.c`
2. `services/src/time_service.c`
3. `services/src/indication_service.c`
4. `bsp/bluepill/src/board_led.c`
5. `bsp/bluepill/src/board_timebase.c`
6. `bsp/bluepill/src/board.c`
7. `system/system_init.c`
8. `system/main.c`
9. `startup/startup_stm32f10x_md.S`
10. `docs/architecture.md`

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

## 14. Troubleshooting

### LED Does Not Blink

Check in this order:

1. verify the firmware reaches `main()`;
2. verify `system_init()` returns `true`;
3. break at `SysTick_Handler()`;
4. inspect whether the handler executes repeatedly;
5. inspect the board time counter;
6. break at `indication_service_toggle()`;
7. verify PC13 output changes.

### LED Is Always On or Always Off

Remember that PC13 is active-low.

Verify:

- board pin mapping;
- GPIO output mode;
- logical active-level translation;
- whether the specific Blue Pill board actually uses PC13 for the LED.

### Debugger Is Difficult to Attach

Verify SWD wiring and OpenOCD:

```text
SWDIO
SWCLK
GND
3.3V reference
```

The supplied configuration uses:

```tcl
reset_config none
```

and the example uses `__NOP()` rather than sleeping in `WFI`.

## 15. Extension Exercises

1. Change the blink period to 100 ms.
2. Add separate ON and OFF durations.
3. Add a second logical indicator.
4. Replace periodic toggle with a small Application state machine.
5. Move the LED to another GPIO while leaving Application unchanged.
6. Use a timer instead of SysTick for the timebase.

## 16. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
- [`../README.md`](../README.md)
