# Architecture — 08-adc-dma

## 1. Hardware Data Path

```text
TIM3 update
    |
    v
ADC1 channel 0
    |
    v
ADC1->DR
    |
    v
DMA1 Channel 1
    |
64-sample circular buffer
```

CPU intervention is not required for each individual sample.

## 2. Software Dependency Graph

```text
Application
    |
    +--> ADC Service
    |       |
    |       v
    |   Board ADC/DMA
    |       |
    |       v
    |   ADC/DMA/TIM/GPIO SPL
    |
    +--> Indication Service
            |
            v
         Board LED
```

## 3. Ownership Table

| Concern | Owner |
|---|---|
| sample timing | TIM3 / Board ADC-DMA |
| ADC conversion | ADC1 / Board ADC-DMA |
| DMA circular buffer | Board ADC-DMA |
| DMA IRQ | Board ADC-DMA |
| stable completed block | Board ADC-DMA |
| average/min/max/mV | ADC Service |
| threshold hysteresis | Application |
| PC13 polarity | Board LED |

## 4. ISR Placement in the Current Code

`DMA1_Channel1_IRQHandler()` lives in the BSP source that owns ADC/DMA.

This matches the repository rule.

The ISR does not call the ADC Service.

## 5. Concurrency Zones

Three memory zones exist:

1. DMA-owned circular buffer;
2. BSP stable completed block;
3. Service-owned processing buffer.

The block-ready flag bridges ISR and thread mode.

A short PRIMASK critical section protects take-and-clear/copy behavior.

## 6. Overrun Semantics

If a new DMA half completes while the previous stable block is still pending:

```text
s_overrun_count++
new completed half replaces previous published block
```

The design prioritizes the newest block over preserving an unbounded backlog.

## 7. Sampling Determinism

Sample timing comes from TIM3 hardware, not super-loop execution.

Therefore:

```text
thread jitter != sample-time jitter
```

as long as the hardware pipeline continues running.

Long interrupt masking can still affect DMA service latency and cause overrun.

## 8. Service Decoupling

ADC Service receives a block of raw samples and converts it into:

```text
average_raw
minimum_raw
maximum_raw
millivolts
sequence
```

Application does not know DMA buffer geometry.

## 9. Error Propagation

DMA transfer errors are counted in BSP and exposed through Service to
Application debug globals.

Calibration/configuration failure causes initialization failure and panic.

## 10. Memory Use

Major static sample storage:

```text
DMA buffer:       64 * 2 = 128 bytes
completed block:  32 * 2 = 64 bytes
Service buffer:   32 * 2 = 64 bytes
```

This intentionally trades RAM for simple ownership and stable processing.

## 11. Extension Options

### Multi-Channel Scan

Interleave channels in the DMA stream and let Service deinterleave them.

### No-Copy Ping-Pong

Process DMA halves directly with strict ownership and timing guarantees.

This reduces copies but increases concurrency complexity.

### Multiple-Block Queue

Queue block descriptors/data if every block must be retained.

This increases RAM and overflow-policy complexity.

## 12. Boundary Rule

Keep this boundary:

```text
hardware sample movement -> BSP
sample interpretation -> Service
product threshold policy -> Application
```

Do not move ADC/DMA register/SPL details upward just to reduce the number of
functions.
