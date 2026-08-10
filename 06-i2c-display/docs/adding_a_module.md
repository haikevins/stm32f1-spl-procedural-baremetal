# Extending 06 - I2C SSD1306 Display

This guide shows how to add functionality while preserving the architecture of
this example.

## Before Adding Code

Identify whether the new behavior is:

| New responsibility | Put it in |
|---|---|
| Product/demo decision | `app/` |
| Hardware-independent capability | `services/` |
| New Blue Pill pin/peripheral | `bsp/bluepill/` |
| New external IC protocol | `ecual/` |
| Generic ring/queue/CRC/type | `common/` |
| Initialization ordering | `system/` |
| Additional SPL driver source | `config/modules.mk` |

## Keep Existing Ownership Intact

Current example focus:

Drive a four-pin 128x64 SSD1306 OLED over I2C1 using a BSP bus layer, ECUAL display driver, framebuffer, text rendering, and a progress-bar demo.

New code should not bypass the existing abstraction merely because a peripheral
pointer is convenient.

## Procedure

1. **Define the API.** Create the header in the correct layer.
2. **Add board mapping.** Put pins, peripheral instance, clock, and IRQ mapping
   in BSP.
3. **Add SPL implementation source.** Update `config/modules.mk`.
4. **Implement low-level behavior.** Keep SPL/CMSIS calls below Services.
5. **Add ECUAL if the new feature is an off-chip device.**
6. **Add a Service** if Application needs a hardware-independent concept.
7. **Connect initialization** in `system/system_init.c`.
8. **Add bounded processing** to Application/Service `process()` functions.
9. **Design ISR handoff** with flags/rings/queues rather than upward callbacks.
10. **Document wiring, timing, test result, and failure behavior.**

## Interrupt Checklist

If the extension uses an IRQ:

- [ ] handler name exactly matches the startup vector;
- [ ] pending peripheral flag is acknowledged correctly;
- [ ] ISR has no delay loop;
- [ ] ISR has no Application include;
- [ ] ISR does not format text or run a protocol state machine;
- [ ] buffer overflow behavior is defined;
- [ ] Service/Application consumes the event in thread mode.

## Configuration Checklist

Put behavior-changing constants in `config/` rather than scattering literals.

Examples include:

- baud rate;
- buffer size;
- debounce time;
- sample rate;
- PWM frequency;
- bus frequency;
- timeout;
- external-device address.

## SPL Source Checklist

After adding a peripheral, make sure its SPL implementation `.c` file is listed
in `config/modules.mk`. Including only the header is not sufficient.

## Validation

```bash
make check-layers
make clean
make
```

Then test both normal operation and at least one failure case.

## Regression Questions

- Does the original example behavior still work?
- Can Application still build without BSP/SPL headers?
- Is any ISR longer than before?
- Is the super-loop still bounded?
- Are new shared variables safe across ISR/thread context?
- Is every new hardware assumption documented?
