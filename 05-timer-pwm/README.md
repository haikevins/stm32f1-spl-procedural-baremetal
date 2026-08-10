# 05-timer-pwm — TIM2 Channel 1 Hardware PWM

## 1. Learning Objectives

This example separates **waveform generation** from **software scheduling**.

You will learn:

- how TIM2_CH1 produces PWM in hardware;
- how APB1 timer clock differs from PCLK1;
- how SPL configures PSC, ARR, and CCR1;
- how PWM mode 1 and preload work;
- why duty is expressed as permille at the Service boundary;
- how SysTick schedules a slow breathing effect without generating PWM edges.

## 2. Wiring

Use an external LED:

```text
PA0 / TIM2_CH1 ---- 330 ohm ---- LED anode
GND --------------------------- LED cathode
```

The onboard PC13 LED is not connected to a PWM timer channel and is not used by
this example.

## 3. Configuration

`config/board_config.h`:

```c
#define BOARD_TIMEBASE_HZ       (1000UL)
#define BOARD_PWM_TIMER_TICK_HZ (1000000UL)
#define BOARD_PWM_FREQUENCY_HZ  (1000UL)
```

`config/pwm_config.h`:

```c
#define PWM_BREATH_UPDATE_PERIOD_MS (10UL)
#define PWM_BREATH_STEP_PERMILLE    (10U)
```

Resulting behavior:

```text
PWM carrier: 1 kHz
Application update: every 10 ms
Duty step: 1%
0 -> 100%: about 1 s
100 -> 0%: about 1 s
```

## 4. Clock Calculation

TIM2 is connected to APB1.

The BSP obtains the clock tree with:

```c
RCC_GetClocksFreq(&clocks);
```

Then:

```text
timer_clock = PCLK1
```

but STM32F1 timers receive twice PCLK when the APB prescaler is not 1.

Therefore the code checks `RCC_CFGR_PPRE1` and doubles the timer clock when
required.

For the common 72 MHz system clock:

```text
PCLK1 = 36 MHz
TIM2 clock = 72 MHz
```

Target timer tick:

```text
1 MHz
```

so:

```text
prescaler divider = 72
PSC = 71
```

PWM frequency:

```text
1 MHz / 1000 = 1 kHz
```

so:

```text
period counts = 1000
ARR = 999
```

## 5. Duty Representation

The Service/API represents duty in permille:

```text
0      = 0%
500    = 50%
1000   = 100%
```

This keeps Application independent from timer period counts.

The BSP converts:

```text
compare_counts =
    (duty_permille * period_counts + 500) / 1000
```

The added 500 provides integer rounding.

## 6. Timer Configuration Sequence

`board_pwm_init()`:

1. enables GPIOA and TIM2 clocks;
2. configures PA0 as alternate-function push-pull;
3. calculates timer input clock;
4. calculates PSC;
5. calculates ARR;
6. configures TIM2 up-counting;
7. configures Channel 1 as PWM mode 1;
8. enables channel output;
9. enables CCR1 preload;
10. enables ARR preload;
11. initializes compare to zero;
12. starts TIM2.

SPL calls include:

```text
TIM_TimeBaseInit
TIM_OC1Init
TIM_OC1PreloadConfig
TIM_ARRPreloadConfig
TIM_SetCompare1
TIM_Cmd
```

## 7. Preload

Preload prevents duty/period changes from taking effect at arbitrary points
inside the current PWM cycle.

With preload enabled, new values are transferred at the timer update boundary.

This produces cleaner PWM updates and avoids a malformed partial pulse when the
Application changes duty.

## 8. Application Fade Algorithm

Application state:

```text
current duty
increasing/decreasing direction
last update timestamp
```

Every 10 ms:

```text
increasing?
    |
    +--> duty += 10 permille
    |
    +--> hit 1000 -> reverse

decreasing?
    |
    +--> duty -= 10 permille
    |
    +--> hit 0 -> reverse
```

The resulting brightness envelope is triangular.

## 9. Scheduling

SysTick provides a 1 ms scheduling timebase.

The Application does not use delays.

It advances:

```c
s_last_update_ms += PWM_BREATH_UPDATE_PERIOD_MS;
```

rather than assigning the current time. This reduces long-term phase drift from
small super-loop scheduling delays.

TIM2 hardware continues generating PWM between Application updates.

## 10. Architecture

```text
Application
    |
    +--> PWM Service ------> Board PWM ------> TIM2/GPIO SPL
    |
    +--> Time Service -----> Board Timebase -> SysTick/CMSIS
```

The Application specifies desired duty and update timing only.

## 11. Interrupts

TIM2 interrupts are not used.

PWM generation is entirely hardware-driven.

The only interrupt required by this example is SysTick for the millisecond
timebase.

## 12. Debug Symbols

Useful state:

```gdb
p s_duty_permille
p s_increasing
p s_last_update_ms
p s_pwm_period_counts
```

Breakpoints:

```gdb
break board_pwm_set_duty_permille
break application_process
```

## 13. Idle Behavior

`system_idle()` uses `__NOP()`.

The CPU remains available for SWD debugging while TIM2 keeps generating PWM
independently.

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

## 14. Test with an Oscilloscope or Logic Analyzer

Probe PA0.

Expected:

```text
frequency: about 1 kHz
period: about 1 ms
duty: slowly changes 0% -> 100% -> 0%
```

A logic analyzer is useful for frequency/duty. An oscilloscope better shows the
analog brightness-related average behavior.

## 15. Troubleshooting

### No Waveform

Check:

- PA0 wiring;
- GPIOA clock;
- TIM2 clock;
- alternate-function mode;
- timer enabled;
- Channel 1 output enabled.

### Frequency Is Off by a Factor of Two

This strongly suggests the APB1 timer x2 rule was handled incorrectly.

Verify PPRE1 and actual TIM2 input clock.

### Duty Does Not Change

Check:

- Application reaches periodic update;
- SysTick increments;
- `pwm_service_set_duty_permille()` is called;
- CCR1 changes.

### LED Is Dim or Fade Is Hard to See

Check LED orientation and resistor value.

Human brightness perception is nonlinear, so a linear duty ramp is not a
perceptually linear brightness ramp.

## 16. Extension Exercises

1. Add gamma-corrected brightness.
2. Change PWM carrier frequency.
3. Use another timer channel.
4. Drive RGB channels with three PWM outputs.
5. Replace triangular breathing with sine-table duty.
6. Use a timer interrupt instead of SysTick for the slow update scheduler.

## 17. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
