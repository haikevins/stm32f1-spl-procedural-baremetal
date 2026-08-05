# Adding a Module

Use the lowest layer that matches the responsibility.

| Responsibility | Location |
|---|---|
| Product state machine | `app/` |
| Time, event, indication, protocol service | `services/` |
| Onboard LED, button, console, timebase | `bsp/bluepill/` |
| External display, sensor, EEPROM | `ecual/` |
| CRC, ring buffer, fixed queue | `common/` |
| Initialization wiring | `system/system_init.c` |
| Required SPL implementation source | `config/modules.mk` |

## Procedure

1. Define the public API in the layer's `include/` directory.
2. Put implementation and private headers in the layer's `src/` directory.
3. Include only APIs from permitted lower layers.
4. Add required SPL source files to `config/modules.mk`.
5. Connect initialization in `system/system_init.c`.
6. Keep application processing non-blocking.
7. Run `make check-layers`.
8. Run `make`.
