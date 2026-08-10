# 05 - Timer PWM

## Purpose

Generate 1 kHz PWM in TIM2 hardware and use a SysTick-scheduled Application state machine to create a 0-100-0% breathing effect.

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


Use an external LED:

```text
PA0 / TIM2_CH1 ---- 330 ohm ---- LED anode
GND --------------------------- LED cathode
```

The onboard PC13 LED is not used for PWM.


## Compile-Time Configuration


| Setting | Value |
|---|---|
| PWM timer | TIM2 |
| Channel | CH1 |
| Output pin | PA0 |
| Timer target tick | 1 MHz |
| PWM frequency | 1 kHz |
| SysTick timebase | 1 kHz |
| Duty unit | permille, 0..1000 |
| Duty update period | 10 ms |
| Duty step | 10 permille |

The ramp therefore takes roughly 1 second from 0 to 100% and another second
back to 0%.


## Initialization Sequence


`board_pwm_init()` determines the TIM2 input clock through
`RCC_GetClocksFreq()`. Because TIM2 is on APB1, it doubles PCLK1 when the APB1
prescaler is not 1.

The BSP then calculates:

```text
prescaler divider = timer_clock / 1 MHz
period counts     = 1 MHz / 1 kHz = 1000
```

TIM2 CH1 is configured for PWM mode 1 with output and preload enabled.

The board then starts the 1 ms SysTick timebase used only to decide when the
Application changes duty.


## Runtime Behavior


TIM2 generates the waveform continuously in hardware.

Every 10 ms the Application changes its desired duty by 10 permille:

```text
0 -> 10 -> ... -> 1000 -> 990 -> ... -> 0 -> repeat
```

`pwm_service_set_duty_permille()` passes the logical duty to the BSP.

The BSP calculates compare counts with rounding:

```text
compare = duty_permille * period_counts / 1000
```

No TIM2 interrupt is needed.


## SPL / Low-Level Behavior


Important SPL concepts:

- `TIM_TimeBaseInit()` sets PSC/ARR;
- `TIM_OC1Init()` selects PWM1;
- `TIM_OC1PreloadConfig()` enables CCR preload;
- `TIM_ARRPreloadConfig()` enables ARR preload;
- `TIM_SetCompare1()` updates duty;
- `TIM_Cmd()` starts TIM2.

SysTick is a separate scheduler timebase and does not generate the PWM waveform.


## Architectural Notes


This example separates scheduling from signal generation. SysTick tells the
Application when to change state; TIM2 owns the precise PWM edges. That pattern
scales better than software-toggling a GPIO at PWM frequency.


## Interrupt and Concurrency Policy

The project follows the repository-wide rule that an interrupt handler belongs
to the lowest module that owns the peripheral. The ISR, when present, may clear
flags, transfer low-level data, and record bounded state. Higher-level policy is
processed later in normal thread mode.

`system_idle()` in this concrete example executes `__NOP()` rather than
`__WFI()`.

## Test Procedure and Expected Result


After flashing, the external LED should smoothly brighten for about one second,
then dim for about one second, repeatedly.

With an oscilloscope or logic analyzer on PA0:

```text
frequency ~ 1 kHz
period    ~ 1 ms
duty      changes gradually from 0 to 100%
```

If the frequency is wrong, inspect the APB1 timer clock and prescaler
calculation rather than only ARR.


## GDB Debugging


```gdb
break board_pwm_set_duty_permille
break application_process
continue
```

In context:

```gdb
p s_duty_permille
p s_increasing
p s_pwm_period_counts
```

You can also inspect TIM2 registers through the CMSIS device definition when
stopped in GDB.



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


To use another PWM pin, select a timer/channel that is actually mapped to that
pin on STM32F103 and update both GPIO and timer definitions. Re-check timer bus
(APB1 vs APB2) and the ×2 timer clock rule.


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
