# 08-adc-dma — TIM3 Trigger + ADC1 + DMA1 Circular Buffer

## 1. Learning Objectives

This example builds a continuous sampled-data pipeline.

You will learn:

- PA0 analog input;
- ADC1 clocking and calibration;
- TIM3 TRGO as an external ADC trigger;
- deterministic 1 kHz sample timing;
- DMA1 Channel 1 circular transfer;
- half-transfer/full-transfer interrupts;
- stable block publishing;
- DMA error/overrun diagnostics;
- thread-mode average/min/max/mV processing;
- hysteresis policy in Application.

## 2. Hardware

Connect a potentiometer:

```text
3.3 V ---- potentiometer ---- GND
                  |
                  +---- PA0 / ADC1_IN0
```

PC13 is used as a threshold indicator.

Do not intentionally drive PA0 above VDDA or below GND.

## 3. Compile-Time Configuration

```c
#define BOARD_ADC_REFERENCE_MV                   (3300UL)
#define BOARD_ADC_MAX_RAW_VALUE                  (4095UL)
#define BOARD_ADC_SAMPLE_RATE_HZ                 (1000UL)
#define BOARD_ADC_TRIGGER_TIMER_TICK_HZ          (1000000UL)
#define BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT        (64U)
#define BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT         (32U)
#define BOARD_ADC_CALIBRATION_TIMEOUT_ITERATIONS (1000000UL)

#define APPLICATION_ADC_LED_ON_THRESHOLD_MV      (1800U)
#define APPLICATION_ADC_LED_OFF_THRESHOLD_MV     (1500U)
```

## 4. Data-Rate Model

Sample rate:

```text
1000 samples/s
```

DMA circular buffer:

```text
64 samples
```

One half-buffer block:

```text
32 samples
```

Therefore one block completes every:

```text
32 / 1000 s = 32 ms
```

Half-transfer and transfer-complete interrupts alternate approximately every
32 ms.

## 5. Timer Trigger

TIM3 is configured as the sample clock source.

The BSP calculates:

```text
timer input clock
    |
target timer tick = 1 MHz
    |
period = 1 MHz / 1 kHz = 1000 counts
```

TIM3 TRGO source:

```text
update event
```

ADC1 converts once per TIM3 update.

There is no TIM3 ISR.

## 6. ADC Input Configuration

PA0 is configured as:

```text
GPIO_Mode_AIN
```

ADC1 regular sequence contains one channel:

```text
ADC_Channel_0
rank 1
sample time 55.5 cycles
```

Scan and continuous modes are disabled because timing comes from the external
timer trigger.

## 7. ADC Clock Selection

The example configures:

```c
RCC_ADCCLKConfig(RCC_PCLK2_Div6);
```

With a common PCLK2 of 72 MHz:

```text
ADC clock = 12 MHz
```

This remains within the STM32F103 ADC clock limit.

If the clock tree is changed, review this divider explicitly.

## 8. ADC Reset/Power/Calibration Sequence

The BSP:

1. enables ADC clock;
2. initializes ADC configuration;
3. enables ADC;
4. starts reset calibration;
5. waits with a bounded iteration timeout;
6. starts calibration;
7. waits with a bounded iteration timeout;
8. enables external trigger conversion;
9. starts TIM3.

Calibration failure causes board initialization to fail.

## 9. DMA Mapping

ADC1 maps to:

```text
DMA1 Channel 1
```

DMA configuration:

```text
direction: peripheral -> memory
peripheral address: ADC1->DR
memory: 64 x uint16_t
peripheral increment: disabled
memory increment: enabled
peripheral size: halfword
memory size: halfword
mode: circular
priority: high
M2M: disabled
```

## 10. DMA Interrupt Events

Enabled:

```text
HT  half transfer
TC  transfer complete
TE  transfer error
```

NVIC preemption priority is 1.

No ADC or TIM3 IRQ is required for normal sampling.

## 11. IRQ Handler and Block Publishing

`DMA1_Channel1_IRQHandler()`:

```text
TE?
    |
increment error count
clear TE

HT?
    |
copy samples 0..31 to completed block
mark ready
clear HT

TC?
    |
copy samples 32..63 to completed block
mark ready
clear TC
```

The ISR does not calculate voltage or LED state.

## 12. Why Copy a Block in the ISR

DMA circular memory will be reused by hardware.

If thread mode processed one DMA half directly while DMA later wrapped around,
the same memory could change during processing.

The example copies the completed half into a stable 32-sample block before
publishing it.

This costs ISR copy time but gives simple ownership.

## 13. Thread-Mode Handoff

`board_adc_dma_take_sample_block()`:

1. saves PRIMASK;
2. disables interrupts;
3. checks block-ready flag;
4. copies the stable block to caller storage;
5. clears block-ready;
6. restores PRIMASK.

Only the short shared-state copy occurs inside the critical section.

## 14. ADC Service Processing

For each 32-sample block the Service calculates:

```text
minimum
maximum
sum
rounded average
millivolts
sequence number
```

Conversion:

```text
mV ~= average_raw * 3300 / 4095
```

The 3300 mV reference is an assumption, not a calibrated VDDA measurement.

## 15. Application Hysteresis

LED policy:

```text
mV >= 1800 -> LED ON
mV <= 1500 -> LED OFF
between     -> keep previous state
```

The 300 mV gap prevents flicker around one threshold.

## 16. Debug Globals

```gdb
p application_adc_average_raw
p application_adc_minimum_raw
p application_adc_maximum_raw
p application_adc_millivolts
p application_adc_sequence
p application_adc_dma_overruns
p application_adc_dma_errors
```

`application_adc_sequence` should keep increasing.

## 17. Interrupt/Symbol Expectations

Expected strong handler:

```text
DMA1_Channel1_IRQHandler
```

Expected unused weak handlers:

```text
ADC1_2_IRQHandler
TIM3_IRQHandler
```

This confirms DMA owns the interrupt completion path.

## 18. Initialization Flow

```text
board_init()
    |
    +--> board_led_init()
    +--> board_adc_dma_init()
            |
            +--> clocks
            +--> ADC clock
            +--> PA0 analog
            +--> DMA1 CH1
            +--> DMA interrupts/NVIC
            +--> ADC1 channel/trigger
            +--> TIM3 TRGO
            +--> ADC enable/calibration
            +--> DMA enable
            +--> ADC external trigger
            +--> TIM3 start

system_init()
    |
    +--> adc_service_init()
    +--> indication_service_init()
    +--> application_init()
```

## 19. Architecture

```text
TIM3 -> ADC1 -> DMA1
                 |
                 v
          Board ADC/DMA
                 |
                 v
            ADC Service
                 |
                 v
            Application
                 |
                 v
        Indication Service
```

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

## 20. Test Procedure

1. Wire potentiometer.
2. Flash firmware.
3. Inspect `application_adc_sequence`; verify it increases.
4. Rotate toward GND; raw should approach 0.
5. Rotate toward 3.3 V; raw should approach 4095.
6. Verify millivolts track position approximately.
7. Cross 1800 mV; LED turns ON.
8. Move below 1500 mV; LED turns OFF.
9. Verify DMA error count stays zero.
10. Verify overrun count normally stays zero.

## 21. Troubleshooting

### Raw Always 0

Check:

- potentiometer wiper to PA0;
- GPIO analog mode;
- ADC channel 0;
- trigger actually running;
- DMA sequence count.

### Raw Always 4095

Check:

- PA0 shorted to 3.3 V;
- wiring;
- ground;
- analog input range.

### DMA IRQ Does Not Run

Check:

- DMA1 clock;
- Channel 1;
- HT/TC interrupt enable;
- NVIC enable;
- ADC conversions;
- TIM3 running.

### `dma_overruns` Increases

Thread mode is not consuming published blocks fast enough.

Possible causes:

- long blocking work;
- debugger halt;
- excessive Service work;
- sample rate too high for current design.

### mV Does Not Match a Meter

The firmware assumes:

```text
VDDA = 3300 mV
```

Real VDDA may differ.

For calibrated voltage, measure/reference VDDA rather than assuming it.

## 22. Extension Exercises

1. Add multi-channel scan.
2. Add RMS calculation.
3. Add low-pass filtering.
4. Remove the block copy with a carefully owned ping-pong design.
5. Queue multiple completed blocks.
6. Add calibrated VDDA measurement.
7. Stream measurements over UART.

## 23. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
