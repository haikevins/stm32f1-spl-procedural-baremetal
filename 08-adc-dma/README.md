# 08 - ADC + DMA

## Purpose

Create a periodic sampled-data pipeline using TIM3 trigger events, ADC1 channel 0, DMA1 Channel 1 circular buffering, half/full DMA interrupts, and thread-mode statistics.

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


Use a potentiometer:

```text
3.3 V ---- potentiometer ---- GND
                  |
                  +---- PA0 / ADC1_IN0
```

The onboard PC13 LED is used as a threshold indicator.

Do not intentionally drive the analog input outside the MCU supply range.


## Compile-Time Configuration


| Setting | Value |
|---|---|
| ADC | ADC1 |
| Channel | 0 |
| Pin | PA0 |
| ADC reference assumption | 3300 mV |
| Raw full scale | 4095 |
| ADC sample time | 55.5 cycles |
| ADC clock configuration | PCLK2 / 6 |
| Trigger timer | TIM3 |
| Timer tick target | 1 MHz |
| Sample rate | 1 kHz |
| DMA | DMA1 Channel 1 |
| DMA mode | circular |
| DMA storage | 64 halfwords |
| Published block | 32 samples |
| IRQ priority | preemption 1, subpriority 0 |
| LED ON threshold | 1800 mV |
| LED OFF threshold | 1500 mV |

The two LED thresholds provide hysteresis.


## Initialization Sequence


`board_adc_dma_init()` performs:

```text
enable DMA1 / GPIOA / ADC1 / TIM3 clocks
    |
configure ADC clock PCLK2/6
    |
PA0 analog input
    |
DMA1 CH1:
    peripheral = ADC1->DR
    memory     = 64-sample buffer
    16-bit peripheral + memory width
    memory increment
    circular mode
    high priority
    HT + TC + TE interrupts
    |
NVIC DMA1_Channel1
    |
ADC1:
    independent
    single regular channel
    external TIM3 TRGO
    right aligned
    55.5-cycle sample time
    |
TIM3:
    computed PSC/ARR
    TRGO = update
    |
ADC calibration
    |
enable external trigger
    |
start TIM3
```

No ADC interrupt or TIM3 interrupt is required.


## Runtime Behavior


Data path:

```text
TIM3 update @ 1 kHz
        |
        v
ADC1 conversion PA0
        |
        v
DMA1 Channel 1 circular buffer [64]
        |
        +--> HT: samples 0..31
        |
        +--> TC: samples 32..63
        |
DMA1_Channel1_IRQHandler
        |
        +--> copy completed half into stable 32-sample block
        +--> mark block ready
        +--> count overrun/error
        |
thread mode
        |
adc_service_process()
        |
        +--> min
        +--> max
        +--> rounded average
        +--> estimated millivolts
        |
Application
        |
        +--> diagnostics
        +--> PC13 hysteresis
```

The ISR intentionally does not calculate statistics or apply LED policy.


## SPL / Low-Level Behavior


DMA uses circular mode and half-transfer/full-transfer interrupts.

The BSP copies the completed half into a separate stable block before
publishing it. If a new half arrives while the previous block is still pending,
the overrun counter increments and the newest completed half replaces the
published block.

Thread mode copies the published block under a short PRIMASK critical section.

Millivolts are estimated from:

```text
mV = average_raw * 3300 / 4095
```

The result is only as accurate as the `3300 mV` VDDA assumption.


## Architectural Notes


This is the clearest producer/consumer example in the repository. Hardware and
DMA continuously produce samples, the BSP converts interrupt completion into a
stable block, the Service converts samples into a measurement, and Application
owns only threshold policy.


## Interrupt and Concurrency Policy

The project follows the repository-wide rule that an interrupt handler belongs
to the lowest module that owns the peripheral. The ISR, when present, may clear
flags, transfer low-level data, and record bounded state. Higher-level policy is
processed later in normal thread mode.

`system_idle()` in this concrete example executes `__NOP()` rather than
`__WFI()`.

## Test Procedure and Expected Result


Rotate the potentiometer slowly.

Use GDB to observe average/raw/mV values. Expected rough points:

```text
0 V     -> raw near 0
1.65 V  -> raw near 2048
3.3 V   -> raw near 4095
```

PC13 behavior:

```text
>= 1800 mV -> LED ON
<= 1500 mV -> LED OFF
1500..1800 -> preserve previous state
```

During normal operation the DMA error count should remain zero. Overrun count
should also remain zero unless thread-mode processing is intentionally delayed.


## GDB Debugging


Application exposes:

```gdb
p application_adc_average_raw
p application_adc_minimum_raw
p application_adc_maximum_raw
p application_adc_millivolts
p application_adc_sequence
p application_adc_dma_overruns
p application_adc_dma_errors
```

Useful breakpoint:

```gdb
break DMA1_Channel1_IRQHandler
continue
```

`application_adc_sequence` should continue increasing as completed 32-sample
blocks are processed.



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


When changing the ADC input, update GPIO pin and ADC channel together. When
changing sample rate, verify TIM3 clock, timer tick divisibility, period range,
ADC conversion time, and DMA processing budget. If VDDA differs from 3.3 V,
update or calibrate the reference used for millivolt conversion.


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
