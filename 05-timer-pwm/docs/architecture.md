# Architecture — 05-timer-pwm

## 1. Dependency Graph

```text
Application
    |
    +--> PWM Service
    |       |
    |       v
    |    Board PWM
    |       |
    |       v
    |    TIM2/GPIO SPL
    |
    +--> Time Service
            |
            v
       Board Timebase
            |
            v
          SysTick
```

## 2. Ownership

| Concern | Owner |
|---|---|
| desired breathing shape | Application |
| logical duty API | PWM Service |
| PA0/TIM2_CH1 mapping | Board PWM |
| timer clock calculation | Board PWM |
| PSC/ARR/CCR configuration | Board PWM |
| 1 ms scheduling time | Board Timebase |

## 3. Hardware vs Software Timing

Two different time scales exist:

```text
1 kHz PWM carrier -> TIM2 hardware
10 ms duty update -> Application scheduling
```

The CPU does not toggle PA0 at 1 kHz.

This separation minimizes timing jitter and CPU load.

## 4. Why Permille at the Service Boundary

Permille:

```text
0..1000
```

is independent from:

- ARR;
- timer width;
- timer input clock;
- PWM frequency.

That makes the Application/Service contract portable across different timer
implementations.

## 5. Clock Ownership

The BSP owns clock-tree interpretation.

Application asks for behavior in milliseconds and logical duty.

The Board PWM converts the current PCLK/timer clock into PSC/ARR.

## 6. Preload Semantics

CCR1 and ARR preload ensure updates are synchronized to timer update events.

Without preload, changing compare mid-period may create one abnormal pulse.

## 7. ISR Policy

No TIM2 ISR is required.

Hardware PWM should remain hardware-driven unless the product explicitly needs
interrupt-time behavior at PWM events.

SysTick ISR remains minimal and only increments time.

## 8. Failure Handling

`board_pwm_init()` validates:

- non-zero timer/frequency configuration;
- exact divisibility for timer tick;
- 16-bit prescaler range;
- 16-bit period range.

Failure propagates:

```text
board_pwm_init() -> false
board_init() -> false
system_init() -> false
system_panic()
```

## 9. Extension Boundary

Good extension points:

- waveform shape -> Application;
- logical duty behavior -> Service;
- timer/channel/pin -> BSP;
- timer implementation -> lower layer/SPL.

## 10. What Not to Do

Avoid:

- calculating CCR values in Application;
- hard-coding a 72 MHz clock in Application;
- software-toggling PA0 for PWM;
- putting a breathing state machine in a timer ISR;
- exposing `TIM_OCInitTypeDef` to Services.
