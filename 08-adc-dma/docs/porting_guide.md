# Porting Guide — 08-adc-dma

## 1. Changing ADC Input Pin/Channel

Update both:

- GPIO analog pin;
- ADC channel number.

These must match the STM32F103 pinout.

Keep Application unchanged.

## 2. Changing Sample Rate

Update:

```c
BOARD_ADC_SAMPLE_RATE_HZ
```

Verify:

```text
timer_tick % sample_rate == 0
```

and confirm ADC conversion time is short enough for the requested rate.

## 3. Changing Trigger Timer

Verify the selected timer can produce an ADC external trigger supported by
STM32F103 ADC1.

Update:

- RCC clock;
- timer instance;
- TRGO selection;
- ADC external trigger selection.

## 4. Changing DMA Buffer Size

The current design requires:

```text
buffer >= 2
buffer even
buffer <= 65535
```

Block size is half the circular buffer.

Recalculate RAM usage and block period.

## 5. Changing DMA Channel

DMA peripheral mapping is fixed by the MCU.

Do not select a channel arbitrarily.

Verify the reference mapping for the new ADC/peripheral.

## 6. Changing the ADC Clock Limit

The current SPL code explicitly selects:

```text
PCLK2 / 6
```

Review the target datasheet before changing the divider.

A different MCU/family may have a different ADC clock specification.

## 7. Changing Reference Voltage

Update:

```c
BOARD_ADC_REFERENCE_MV
```

only if the assumption is truly valid.

For accurate measurement, measure VDDA or use an internal reference-based
calibration strategy.

## 8. Multi-Channel ADC

Enable scan mode and configure multiple ranks.

Then define DMA data layout clearly:

```text
CH0, CH1, CH0, CH1, ...
```

Service should own channel extraction/aggregation.

## 9. Changing IRQ Priority

Review all interrupts in the final product.

The DMA ISR must run often enough to publish blocks before data ownership is
lost, but it should still remain short.

## 10. Refactoring ISR Ownership

If a reusable DMA abstraction is introduced, the strong DMA handler may move to
that lower layer.

Preserve the rule:

```text
ISR lives with the lowest owner of DMA1 Channel 1
```

Do not move the handler upward into Service/Application.

## 11. Validation with Oscilloscope/Debug Pin

For precise sample-rate validation, toggle a spare debug pin at block publish or
use timer output where possible.

Measure the expected block cadence:

```text
32 ms per published block at 1 kHz sampling
```

## 12. Validation Checklist

- [ ] PA0/channel mapping correct;
- [ ] ADC clock within limit;
- [ ] calibration completes;
- [ ] TIM3 frequency correct;
- [ ] ADC conversions triggered externally;
- [ ] DMA1 CH1 transfers;
- [ ] HT/TC alternate;
- [ ] sequence increases;
- [ ] errors zero;
- [ ] overruns zero at normal load;
- [ ] voltage trend matches input;
- [ ] hysteresis works.

## 13. Common Pitfalls

- using a GPIO digital mode instead of analog;
- wrong ADC channel for the pin;
- ADC clock too fast;
- enabling continuous mode accidentally;
- wrong external trigger;
- wrong DMA channel;
- forgetting circular mode;
- doing statistics in the ISR;
- ignoring block overrun;
- assuming 3.300 V reference is exact.
