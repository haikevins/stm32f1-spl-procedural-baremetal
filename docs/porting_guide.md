# Porting Guide — Template

## 1. Case A — New Project on the Same Blue Pill

Copy the template and keep startup/linker/vendor layers unchanged.

Change:

- board resources;
- required SPL modules;
- Services;
- Application;
- configuration.

## 2. Case B — Different Board, Same STM32F103C8T6

Usually keep:

```text
app/
services/
ecual/
common/
startup/
linker/
third_party/
```

Replace or review:

```text
bsp/
config/
tools/openocd/
```

## 3. Case C — STM32F103 with Different Memory Density

Update:

- linker flash/SRAM size;
- device density build define;
- startup/vector selection if required;
- OpenOCD/device assumptions.

Do not trust a C8 linker map for a larger/smaller density.

## 4. Case D — Different STM32F1 Part

Review:

- peripheral availability;
- GPIO alternate-function mapping;
- IRQ list;
- startup file;
- density define;
- linker memory;
- SPL device support.

Application/Services can often remain unchanged.

## 5. Case E — Different MCU Family but Still Cortex-M

The architecture can remain, but SPL is STM32F1-specific.

Replace:

- low-level vendor layer;
- BSP implementation;
- startup;
- linker;
- clock configuration;
- OpenOCD target.

Keep higher layers where their APIs are truly hardware-independent.

## 6. Case F — Different CPU Architecture

Port:

- startup;
- linker;
- interrupt model;
- critical-section implementation;
- toolchain flags;
- vendor/device layer.

The conceptual Application/Service/BSP separation can still survive.

## 7. BSP Selection Strategy

Treat BSP as the place where one logical resource is mapped to one physical
board resource.

Examples:

```text
STATUS_LED -> PC13
USER_BUTTON -> PA0
MEMORY_SPI -> SPI1
```

Do not encode board pins in Application.

## 8. Clock Porting

Verify:

- HSE frequency;
- `HSE_VALUE`;
- `SystemInit()`;
- `SystemCoreClock`;
- PCLK1/PCLK2;
- APB timer x2 rule;
- peripheral baud/timer calculations.

## 9. GPIO Porting

For each pin verify:

- port clock;
- pin number;
- input/output/AF mode;
- output speed;
- pull-up/pull-down;
- active polarity.

## 10. Interrupt Porting

Verify:

- vector name;
- IRQ number;
- EXTI grouping;
- NVIC priority;
- pending flag clear sequence;
- startup vector coverage.

## 11. DMA Porting

DMA mappings are device-specific.

Verify:

- peripheral-to-channel mapping;
- transfer width;
- circular/normal mode;
- memory increment;
- half/full/error interrupt flags.

## 12. External-Device Driver Portability

Keep ECUAL unchanged when:

- the off-chip device is unchanged;
- board bus semantics are unchanged.

Replace only the board bus implementation.

## 13. Linker Porting

Update:

- FLASH origin/length;
- RAM origin/length;
- stack top;
- section placement.

Then verify `.data`, `.bss`, stack, and code sizes.

## 14. Startup Porting

The startup file must match:

- exception table;
- MCU peripheral vectors;
- reset sequence;
- architecture instruction set.

## 15. Toolchain Flags

Review:

```text
-mcpu
-mthumb
device density defines
HSE_VALUE
vendor include paths
```

## 16. OpenOCD

Change the target configuration if the MCU changes.

Keep adapter speed conservative during first bring-up.

## 17. Debug Reset Strategy

The current project uses:

```tcl
reset_config none
```

because NRST may not be wired.

If the new probe/board exposes NRST, you may adopt a hardware-reset strategy
after validating it.

## 18. Validation by Layer

### Startup

- reset reaches `main`;
- `.data` initialized;
- `.bss` zeroed.

### Clock

- `SystemCoreClock` correct;
- peripheral bus clocks correct.

### GPIO

- correct pin electrical mode;
- output active level correct.

### Peripheral

- flags/interrupts/data movement correct.

### Service/Application

- logical behavior correct without lower-layer dependencies.

## 19. Automated Checks

Run:

```bash
make check-layers
make clean
make
make size
```

## 20. Port Acceptance Checklist

- [ ] Linker matches memory.
- [ ] Startup matches vector table.
- [ ] Clock tree validated.
- [ ] BSP pins validated.
- [ ] SPL source list validated.
- [ ] Interrupt names/priorities validated.
- [ ] DMA mapping validated if used.
- [ ] OpenOCD can connect.
- [ ] Layer checker passes.
- [ ] Clean build passes.
- [ ] Hardware test reproduces expected behavior.
