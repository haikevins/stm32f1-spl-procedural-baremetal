# Porting Guide — 02-gpio-input-interrupt

## 1. Moving the Button to Another Pin on STM32F103

Update together:

- GPIO port;
- GPIO pin;
- GPIO clock;
- AFIO port source;
- EXTI pin source;
- EXTI line;
- IRQ vector.

Example: moving from PA0 to PB8 also changes the handler group because EXTI8
uses `EXTI9_5_IRQHandler()`.

## 2. Changing Polarity

For an active-high button:

- configure a pull-down or external bias as appropriate;
- select the correct EXTI edge;
- update `BOARD_USER_BUTTON_ACTIVE_LOW`;
- verify `board_button_is_pressed()`.

Service/Application logic should remain unchanged.

## 3. Using an External Pull-Up

Change GPIO mode from internal pull-up to a floating/input mode appropriate for
the external resistor network.

Keep the logical pressed polarity explicit.

## 4. Changing Debounce

Update:

```c
#define BUTTON_DEBOUNCE_TIME_MS (...)
```

Shorter values react faster but may allow bounce. Longer values reject more
bounce but delay event publication.

Do not convert debounce into a blocking delay.

## 5. Changing IRQ Priority

Review the whole project priority scheme before changing priority.

Higher urgency should be justified by latency requirements, not convenience.

The handler remains short regardless of priority.

## 6. Porting to Another STM32F1

Verify:

- GPIO/AFIO availability;
- EXTI routing;
- vector name;
- NVIC implementation;
- SPL device support;
- startup vector table.

## 7. Porting to Another MCU Family

Preserve:

```text
raw edge -> Button Service debounce -> Application event
```

Replace:

- GPIO implementation;
- external interrupt controller;
- NVIC/vendor layer;
- startup/vector names.

## 8. Verification Checklist

-  released input has a stable idle level;
-  one physical press produces an interrupt;
-  pending flag clears;
-  raw event transfers to thread mode;
-  debounce delay is correct;
-  one press produces one logical event;
-  LED toggles only through Indication Service.

## 9. GDB Checklist

Break at:

```gdb
break EXTI0_IRQHandler
break button_service_process
break indication_service_toggle
```

Inspect the raw flag, debounce state, and pending Service event.

## 10. Logic Analyzer/Oscilloscope

Probe PA0 if bounce behavior is unclear.

Compare:

```text
physical bouncing edge train
```

with:

```text
single logical Application action
```

The difference demonstrates why debounce belongs outside the ISR.

## 11. Common Mistakes

- changing GPIO pin but not EXTI port source;
- using the wrong grouped EXTI handler;
- forgetting input bias;
- clearing the pending flag incorrectly;
- calling Application from ISR;
- busy-waiting 30 ms in ISR;
- reading and clearing shared state without atomicity.
