# Example 08: ADC with DMA

A layered STM32F103C8T6 Blue Pill example using CMSIS and the STM32F10x
Standard Peripheral Library.

ADC1 samples `PA0 / ADC1_IN0` at 1 ksample/s. TIM3 generates the conversion
trigger and DMA1 Channel 1 continuously transfers conversion results into a
64-sample circular buffer.

DMA half-transfer and transfer-complete interrupts publish 32-sample blocks.
The ADC Service calculates the average, minimum, maximum, and approximate
input voltage outside the ISR.

## Hardware wiring

Use a potentiometer or another 0..3.3 V analog source:

```text
3.3 V ---- potentiometer ---- GND
                 |
                 +---- Blue Pill PA0 / ADC1_IN0
```

Never apply a voltage below GND or above 3.3 V to PA0.

The onboard active-low LED on PC13 is used as a threshold indicator:

- LED turns on at 1.8 V or above;
- LED turns off at 1.5 V or below;
- the gap provides hysteresis.

## Peripheral configuration

| Resource | Configuration |
|---|---|
| ADC | ADC1 regular channel 0 |
| Analog pin | PA0 |
| ADC resolution | 12 bit, right aligned |
| ADC clock | PCLK2 / 6, normally 12 MHz |
| Sample time | 55.5 ADC cycles |
| Conversion trigger | TIM3 TRGO update |
| Sample rate | 1 ksample/s |
| DMA | DMA1 Channel 1 |
| DMA mode | Circular |
| DMA buffer | 64 halfwords |
| Published block | 32 samples |
| DMA interrupts | Half-transfer, complete, error |
| Idle policy | `__NOP()` |

## Runtime flow

```text
TIM3 update at 1 kHz
    -> ADC1 conversion
        -> DMA1 Channel 1
            -> 64-sample circular buffer
                -> HT/TC interrupt
                    -> stable 32-sample low-level block
                        -> ADC Service
                            -> average/min/max/millivolts
                                -> Application
                                    -> PC13 threshold LED
```

## Dependency paths

```text
Application
    -> ADC Service
        -> Board ADC DMA
            -> GPIO / RCC / ADC1 / DMA1 / TIM3 / NVIC

Application
    -> Indication Service
        -> Board LED
            -> GPIO
```

Application code does not include BSP, SPL, CMSIS, or STM32 device headers.

## Interrupt policy

`DMA1_Channel1_IRQHandler()` remains in the BSP because the BSP owns ADC1
and DMA1 Channel 1.

The ISR only:

1. acknowledges DMA flags;
2. copies the completed DMA half into one static low-level block;
3. publishes one pending-block flag;
4. increments error or overrun counters.

It does not calculate voltage, update the LED, or call Application/Service
functions.

If Application fails to consume a completed block before the next block is
published, the newest block replaces the old block and
`application_adc_dma_overruns` increments.

## GDB diagnostics

The example intentionally avoids adding UART just for diagnostics. Inspect:

```gdb
print application_adc_average_raw
print application_adc_minimum_raw
print application_adc_maximum_raw
print application_adc_millivolts
print application_adc_sequence
print application_adc_dma_overruns
print application_adc_dma_errors
```

Expected approximate values:

```text
PA0 = 0.0 V  -> raw around 0
PA0 = 1.65 V -> raw around 2048
PA0 = 3.3 V  -> raw around 4095
```

The millivolt result assumes the analog reference is exactly 3300 mV. It is
therefore an estimate unless VDDA is measured or calibrated.

## Why use TIM3 as the trigger?

ADC continuous mode can sample far faster than a simple super-loop needs.
TIM3 gives the acquisition an explicit and repeatable 1 ksample/s rate while
DMA performs every sample transfer without CPU polling.

## Idle policy

`system_idle()` uses `__NOP()` rather than `__WFI()`:

```c
void system_idle(void)
{
    __NOP();
}
```

This keeps flashing and debugging reliable with an ST-Link that has no NRST
connection. ADC, TIM3, DMA, and their interrupts continue operating normally.

## Build

```bash
make check-layers
make clean
make
```

## Flash

```bash
make flash
```

## Configuration

- sample rate, reference voltage, and buffer size:
  `config/board_config.h`
- LED thresholds:
  `config/application_config.h`
- PA0/ADC1/DMA1/TIM3 mapping:
  `bsp/bluepill/src/board_pins.h`
- selected SPL modules:
  `config/modules.mk`

## Selected SPL modules

```text
misc.c
stm32f10x_adc.c
stm32f10x_dma.c
stm32f10x_gpio.c
stm32f10x_rcc.c
stm32f10x_tim.c
```

## License

This example is licensed under the MIT License.
