# Porting Guide — 01-blink-led

## 1. Porting Goal

A successful port should preserve:

```text
Application -> Time Service / Indication Service
```

while replacing only the hardware-specific implementation required by the new
board or MCU.

## 2. Porting to Another Blue Pill with the Same STM32F103

Normally no source changes are required if:

- the onboard LED is still PC13;
- the system clock setup is compatible;
- SWD/OpenOCD settings are unchanged.

Build and hardware-test before assuming board clones are electrically
identical.

## 3. Moving the LED to Another Pin

Update BSP pin definitions and GPIO clock.

Verify:

- new GPIO port;
- new pin;
- RCC peripheral clock;
- output mode;
- active-low/active-high behavior.

Application and Indication Service should remain unchanged.

## 4. Changing the System Clock

Verify:

- `SystemInit()`;
- `SystemCoreClockUpdate()`;
- HSE definition;
- clock tree.

The SysTick reload is derived from `SystemCoreClock`, so do not hard-code a new
reload in Application.

## 5. Changing the Timebase Frequency

If the Time Service still claims millisecond units, keep the physical timebase
at 1 kHz.

If you change the physical tick frequency, either:

- convert ticks to milliseconds in the BSP/Service, or
- change the API semantics and every consumer consistently.

## 6. Using a Timer Instead of SysTick

Replace only the Board Timebase implementation.

The Service can keep:

```c
uint32_t time_service_get_ms(void);
```

The Application should not care whether time comes from SysTick, TIM2, or
another timer.

## 7. Porting to Another STM32F1 MCU

Review:

- startup vector table;
- linker memory;
- system clock code;
- GPIO availability;
- SPL density/device defines;
- SysTick/CMSIS compatibility.

## 8. Porting to Another MCU Family

Keep Application and Service APIs if possible.

Replace:

- BSP;
- vendor peripheral layer;
- startup;
- linker;
- clock implementation;
- debug target configuration.

## 9. Validation Checklist

- [ ] `make check-layers` passes.
- [ ] reset reaches `main()`.
- [ ] timebase increments at 1 ms.
- [ ] logical LED OFF is correct at startup.
- [ ] LED toggles every 500 ms.
- [ ] one full blink period is about one second.
- [ ] GDB can attach reliably.

## 10. Common Mistakes

- forgetting active-low polarity;
- enabling the wrong GPIO clock;
- assuming `SystemCoreClock` is correct without updating it;
- changing the tick frequency but keeping millisecond names;
- moving hardware calls into Application;
- using a blocking delay to preserve blink behavior.

## 11. Target State After Porting

The desired final dependency still looks like:

```text
Application
    |
Services
    |
new BSP
    |
new low-level peripheral implementation
```

Only hardware-specific layers should know the new pin or MCU.
