# STM32F1 SPL Procedural Bare-Metal Examples

## 1. Philosophy of the Example Series

The examples are intentionally progressive rather than independent feature
demos.

Every project preserves the same high-level rules:

```text
Application -> Services -> BSP/ECUAL -> SPL/CMSIS -> Hardware
```

Only the peripheral topic changes.

The objective is to learn both **how the STM32 peripheral works** and **where
that behavior belongs architecturally**.

## 2. Recommended Learning Roadmap

### Step 1 — GPIO and Timebase

Start with `01-blink-led`.

Learn:

- active-low board resources;
- GPIO output initialization;
- SysTick timebase;
- non-blocking periodic scheduling;
- logical indicator Service.

### Step 2 — Input Interrupt

Continue with `02-gpio-input-interrupt`.

Learn:

- internal pull-up;
- AFIO/EXTI mapping;
- falling-edge interrupt;
- ISR-to-thread event handoff;
- debounce outside the ISR;
- short critical sections.

### Step 3 — UART Polling

Use `03-uart-polling`.

Learn:

- USART1 GPIO modes;
- 115200 8N1;
- RXNE/TXE polling;
- `try_read`/`try_write`;
- non-blocking echo state.

### Step 4 — UART Interrupt + Ring Buffer

Use `04-uart-interrupt-ring-buffer`.

Learn:

- RX/TX rings;
- single-producer/single-consumer ownership;
- TXE interrupt start/stop;
- RX overflow;
- UART error counters;
- bounded Application processing.

### Step 5 — Timer PWM

Use `05-timer-pwm`.

Learn:

- TIM2_CH1;
- APB1 timer clock x2 rule;
- PSC/ARR/CCR concepts;
- hardware PWM;
- preload;
- separating waveform generation from scheduling.

### Step 6 — I2C + External Device

Use `06-i2c-display`.

Learn:

- PB6/PB7 I2C1 wiring;
- open-drain bus;
- bounded polling;
- SSD1306 command/data control bytes;
- framebuffer rendering;
- ECUAL ownership.

### Step 7 — SPI Flash

Use `07-spi-memory`.

Learn:

- SPI1 mode 0;
- software chip select;
- W25Q64 JEDEC ID;
- BUSY/WEL;
- 4 KiB sector erase;
- 256-byte page-program constraints;
- destructive self-test.

### Step 8 — ADC + DMA Pipeline

Use `08-adc-dma`.

Learn:

- timer-triggered ADC;
- ADC calibration;
- circular DMA;
- half/full-transfer interrupts;
- block handoff;
- average/min/max/mV processing;
- hysteresis.

## 3. Summary Table

| Example | Main hardware | Interrupts | Main software pattern |
|---|---|---|---|
| 01 | PC13 + SysTick | SysTick | periodic Service |
| 02 | PA0 + EXTI0 | SysTick, EXTI0 | edge event + debounce |
| 03 | USART1 | none | polling |
| 04 | USART1 | USART1 | RX/TX ring buffers |
| 05 | TIM2_CH1 + SysTick | SysTick | hardware PWM + scheduled duty |
| 06 | I2C1 + SSD1306 | SysTick | framebuffer + bounded polling |
| 07 | SPI1 + W25Q64 | SysTick | synchronous NOR transactions |
| 08 | TIM3 + ADC1 + DMA1 CH1 | DMA1 CH1 | sampled-data block pipeline |

## 4. Wiring Summary

### SWD — Used by Every Example

```text
ST-Link      Blue Pill
----------------------
SWDIO   ---> PA13 / SWDIO
SWCLK   ---> PA14 / SWCLK
GND     ---> GND
3.3V    ---> 3.3V reference
```

### Example 02 — Button

```text
PA0 ---- push button ---- GND
```

PA0 uses the internal pull-up.

### Examples 03/04 — USB-UART

```text
PA9  USART1_TX  ---> USB-UART RX
PA10 USART1_RX  <--- USB-UART TX
GND              --- USB-UART GND
```

Use a 3.3 V adapter and `115200 8N1`.

### Example 05 — PWM LED

```text
PA0 / TIM2_CH1 ---- 330 ohm ---- LED ---- GND
```

### Example 06 — OLED I2C

```text
Blue Pill      SSD1306
----------------------
3.3V       ---> VCC
GND        ---> GND
PB6        ---> SCL
PB7        ---> SDA
```

### Example 07 — W25Q64

```text
STM32F103C8T6       W25Q64
--------------------------------
3.3V        ------  VCC
GND         ------  GND
PA4         ------  CS
PA5         ------  CLK
PA6         ------  D1 / DO / MISO
PA7         ------  D0 / DI / MOSI
```

### Example 08 — Analog Input

```text
3.3 V ---- potentiometer ---- GND
                  |
                  +---- PA0 / ADC1_IN0
```

## 5. Common Build/Flash Flow

From an example directory:

```bash
make check-layers
make clean
make
make flash
```

Useful targets:

```bash
make size
make tree
make erase
```

## 6. Common Debug Flow

Terminal 1:

```bash
make debug-server
```

Terminal 2:

```bash
make debug
```

The OpenOCD configuration uses `reset_config none`.

## 7. Common Clock Behavior

The examples call CMSIS/SPL clock helpers instead of assuming one universal
peripheral clock.

Important STM32F1 rules:

- core timebase derives from `SystemCoreClock`;
- TIM2/TIM3 are on APB1;
- an APB timer receives 2 x PCLK when its APB prescaler is not 1;
- USART1 and SPI1 are on APB2;
- I2C1 is on APB1;
- ADC1 is on APB2 and must respect the ADC clock limit.

## 8. Architecture and Layer Checker

The source dependency direction is:

```text
Application
    |
    v
Services
    |
    +--> BSP
    |
    +--> ECUAL
             |
             v
      Board bus APIs
             |
             v
      SPL / CMSIS
```

Run:

```bash
make check-layers
```

If the checker rejects an include, treat that as an architecture problem first,
not as a tooling inconvenience.

## 9. Interrupt Ownership and Thread Mode

Interrupts are intentionally low-level.

Examples:

- EXTI0 captures the button edge only.
- USART1 moves bytes between hardware and rings.
- DMA1 CH1 publishes completed ADC blocks.
- SysTick increments the board timebase.

Debounce, echo policy, display rendering, memory verification, ADC statistics,
and LED threshold decisions remain in normal thread mode.

## 10. `system_idle()` in the Example Series

All concrete examples use `__NOP()` in `system_idle()`.

This keeps SWD interaction simple for the ST-Link setup used with
`reset_config none`.

The separate `template/` currently demonstrates `__WFI()`, so do not assume the
template and completed examples use the same idle policy.

## 11. Choosing an Example to Extend

Use the nearest architecture, not merely the nearest peripheral:

- simple periodic task -> Example 01;
- edge-driven input -> Example 02;
- polling byte stream -> Example 03;
- interrupt-driven byte stream -> Example 04;
- hardware waveform -> Example 05;
- I2C external device -> Example 06;
- SPI memory/device -> Example 07;
- high-rate sampled data -> Example 08.

## 12. Rules When Copying Code Between Examples

When reusing code:

1. copy complete module boundaries, not isolated SPL calls;
2. copy required configuration;
3. copy the required SPL source entries in `config/modules.mk`;
4. preserve ISR ownership;
5. re-check peripheral clock assumptions;
6. run the layer checker;
7. re-test electrical wiring.

## 13. Hardware Safety Notes

- Use 3.3 V logic with STM32F103 peripherals.
- Always share ground between boards/adapters.
- Use a resistor with external LEDs.
- Do not intentionally drive PA0 analog input above VDDA or below GND.
- Verify I2C pull-ups.
- Do not connect W25Q64 VCC to 5 V.
- Remember that Example 07 erases the final 4 KiB sector on every reset.

## 14. Detailed Documentation

Each example contains:

```text
README.md
docs/architecture.md
docs/porting_guide.md
```

Read the README for bring-up, then the architecture document for ownership and
the porting guide before moving pins/peripherals.
