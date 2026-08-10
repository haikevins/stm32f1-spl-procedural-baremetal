# Porting Guide — 05-timer-pwm

## 1. Changing Pin/Channel on the Same Timer

Choose a valid TIM2 channel/pin mapping.

Update:

- GPIO pin;
- timer channel;
- corresponding SPL OC init/preload/set-compare functions.

Verify alternate-function mapping.

## 2. Moving to Another Timer

Review:

- APB1 vs APB2;
- timer input clock;
- counter width;
- channel mapping;
- RCC enable;
- SPL function usage.

Application/PWM Service should remain unchanged.

## 3. Changing PWM Frequency

Update:

```c
BOARD_PWM_FREQUENCY_HZ
```

Verify:

```text
timer_tick % pwm_frequency == 0
```

and that resulting period counts fit the timer.

## 4. Changing Timer Resolution

Changing `BOARD_PWM_TIMER_TICK_HZ` affects the representable period and compare
resolution.

Verify PSC and period ranges together.

## 5. Changing Fade Speed

Change:

```text
PWM_BREATH_UPDATE_PERIOD_MS
PWM_BREATH_STEP_PERMILLE
```

Approximate one-way ramp duration:

```text
1000 / step * update_period
```

## 6. Active-Low PWM

If the external load is active-low, handle polarity in the BSP/timer output
configuration rather than changing Application duty semantics.

## 7. Porting to Another MCU Family

Keep:

```text
Application -> PWM Service
```

Replace Board PWM and low-level timer implementation.

Re-check timer clock-tree behavior because the STM32F1 APB timer x2 rule may
not apply identically.

## 8. Validation Checklist

- [ ] expected timer input clock measured/derived;
- [ ] PWM frequency correct;
- [ ] 0% really produces inactive output;
- [ ] 100% really produces continuous active output;
- [ ] duty changes smoothly;
- [ ] preload behavior correct;
- [ ] no timer ISR unexpectedly enabled;
- [ ] layer checker passes.

## 9. Common Pitfalls

- forgetting APB timer x2;
- using the wrong channel function;
- selecting a pin not mapped to that timer channel;
- period count exceeding timer width;
- using 1000 directly as ARR instead of period-count minus one;
- performing timer-register math in Application.
