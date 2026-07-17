# 01 - Blink LED

A procedural bare-metal Blink example for an STM32F103-compatible Blue Pill board using:

- CMSIS Cortex-M3 and STM32F10x device support
- STM32F10x Standard Peripheral Library (SPL) V3.5.0
- GNU Arm Embedded Toolchain (`arm-none-eabi-*`)
- GNU Make
- OpenOCD 0.12.x
- an ST-LINK/V2-style **four-wire** probe: `VCC`, `SWDIO`, `SWCLK`, `GND`

The onboard LED is assumed to be connected to **PC13** and wired **active-low**. Edit `bsp/src/bsp_led.c` if your board uses another LED pin or polarity.

## Behavior

- `SystemInit()` configures the MCU clock through the ST CMSIS system file.
- SysTick provides a 1 ms time base.
- The application toggles PC13 every 500 ms.
- The main loop is non-blocking; it does not use a calibrated busy-wait delay.

```text
Reset_Handler
    -> SystemInit()
    -> main()
        -> App_Init()
            -> BSP_Init()
                -> BSP_LED_Init()
            -> System_Time_Init()
        -> while (1)
            -> App_Run()
                -> BSP_LED_Toggle() every 500 ms
```

## Project structure

```text
01-blink-led/
├── app/
├── bsp/
├── system/
├── linker/STM32F103C8Tx_FLASH.ld
├── scripts/
│   ├── debug.sh
│   ├── flash.sh
│   └── openocd.cfg
├── docs/architecture.md
├── third_party/
├── build/.gitignore
├── Makefile
└── README.md
```

## Target memory map

The linker script deliberately keeps the conservative STM32F103C8 memory map:

| Region | Start | Size |
|---|---:|---:|
| Flash | `0x08000000` | 64 KiB |
| SRAM | `0x20000000` | 20 KiB |

The build defines `STM32F10X_MD`. Keep this linker limit even if a compatible or remarked Blue Pill chip reports a larger physical Flash size through OpenOCD.

## Requirements

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install make gcc-arm-none-eabi binutils-arm-none-eabi openocd
```

Verify the tools:

```bash
arm-none-eabi-gcc --version
arm-none-eabi-objcopy --version
openocd --version
make --version
```

This configuration expects an OpenOCD installation containing:

```text
/usr/share/openocd/scripts/interface/stlink-dap.cfg
```

Check it with:

```bash
ls /usr/share/openocd/scripts/interface/stlink-dap.cfg
```

## ST-LINK/V2 four-wire wiring

Use 3.3 V signaling and follow the labels printed on your specific probe:

| ST-LINK | Blue Pill |
|---|---|
| `VCC` | `3.3V` |
| `GND` | `GND` |
| `SWDIO` | `PA13 / SWDIO` |
| `SWCLK` | `PA14 / SWCLK` |

This four-wire probe does **not** expose `NRST`. Consequently:

- OpenOCD cannot perform hardware connect-under-reset;
- the target must be reset or power-cycled manually;
- recovery/programming uses `BOOT0=1` to boot System Memory;
- `make flash` intentionally does not reset the MCU after verification.

Do not connect 5 V to a 3.3 V target supply/debug pin. Avoid powering the target from two independent sources unless their power arrangement is explicitly designed for it, and always share ground.

## OpenOCD configuration

`scripts/openocd.cfg` uses the native ST-LINK DAP driver:

```tcl
source [find interface/stlink-dap.cfg]
transport select dapdirect_swd
source [find target/stm32f1x.cfg]
adapter speed 100
reset_config none
```

The native DAP driver is used instead of the legacy HLA `stlink.cfg` path. It was validated with an ST-LINK/V2 reporting firmware `V2J45` and a target returning `SWD DPIDR 0x2ba01477`.

## Build

From the project directory:

```bash
make
```

Generated files:

```text
build/01-blink-led.elf
build/01-blink-led.hex
build/01-blink-led.bin
build/01-blink-led.map
build/01-blink-led.lst
```

Release build:

```bash
make clean
make BUILD_TYPE=release
```

Other build commands:

```bash
make size
make disasm
make rebuild
make clean
make help
```

## Probe the target

Because there is no `NRST` wire, use the recovery boot mode before probing:

```text
1. Remove target power.
2. Set BOOT0=1 and BOOT1=0.
3. Restore power or press the board RESET button.
4. Run make probe.
```

```bash
make probe
```

A successful connection contains messages similar to:

```text
SWD DPIDR 0x2ba01477
Cortex-M3 ... processor detected
target ... halted
```

A program counter in the `0x1FFF....` range is expected while booting from System Memory.

## Flash with the four-wire ST-LINK/V2

### Step 1: enter System Memory boot mode

```text
BOOT0 = 1
BOOT1 = 0
```

Then power-cycle the board or press and release RESET. BOOT pin values are sampled during reset; moving the jumper without resetting is not enough.

### Step 2: build, erase, program, and verify

```bash
make flash
```

Equivalent wrapper:

```bash
./scripts/flash.sh
```

The flash target performs:

```text
init
-> halt
-> flash write_image erase <ELF>
-> verify_image <ELF>
-> shutdown
```

It deliberately omits `reset` because the probe has no `NRST` signal and the board is still configured to boot System Memory.

Success is confirmed by output similar to:

```text
wrote ... bytes from file ...
verified ... bytes in ...
```

### Step 3: run the application from Main Flash

After verification:

```text
1. Remove power or unplug the ST-LINK USB cable.
2. Set BOOT0=0 and BOOT1=0.
3. Restore power.
4. Press and release RESET.
```

The MCU now boots from `0x08000000`, and the PC13 LED should toggle every 500 ms.

## Mass erase

Enter System Memory boot mode first:

```text
BOOT0=1, BOOT1=0, then reset/power-cycle
```

Run:

```bash
make erase
```

## Reset behavior

The probe only has four wires, so this command does not attempt an OpenOCD hardware reset:

```bash
make reset
```

It prints the required manual action. Press the board RESET button or power-cycle the board.

## Debug with GDB

After a valid application has been flashed, try booting normally with `BOOT0=0`, then run:

```bash
./scripts/debug.sh
```

Or use two terminals.

Terminal 1:

```bash
make openocd
```

Terminal 2:

```bash
make gdb
```

The GDB helper connects and sends `monitor halt`; it does not request a hardware reset.

Typical commands:

```gdb
break main
continue
next
step
info registers
monitor halt
```

If attaching with `BOOT0=0` fails, return to `BOOT0=1`, reset/power-cycle, and use `make probe` or `make flash`. A probe exposing `NRST` is recommended for convenient connect-under-reset and repeatable source debugging.

## Troubleshooting

### `init mode failed (unable to connect to the target)`

Check in this order:

1. Set `BOOT0=1`, `BOOT1=0`.
2. Power-cycle or press RESET after changing BOOT0.
3. Confirm `SWDIO -> PA13` and `SWCLK -> PA14`.
4. Replace loose Dupont wires and check the SWD header solder joints.
5. Keep `adapter speed 100` in `scripts/openocd.cfg`.
6. Confirm that `stlink-dap.cfg` exists.

### Legacy HLA reports an unexpected ID code

Do not change this project back to:

```tcl
source [find interface/stlink.cfg]
transport select hla_swd
```

On the tested board, that route reports:

```text
UNEXPECTED idcode: 0x2ba01477
expected: 0x1ba01477
```

The native `stlink-dap.cfg` configuration connects successfully.

### Firmware verifies but does not run

- Set `BOOT0=0` and `BOOT1=0`.
- Remove and restore power after changing BOOT0.
- Press RESET.
- Confirm that the onboard LED is PC13 and active-low.

### OpenOCD reports 256 KiB Flash

Some Blue Pill-compatible or remarked devices report a different device ID and a larger Flash size. This example still links to 64 KiB, matching the documented STM32F103C8 capacity and preventing accidental use of unverified memory.

## Design notes

- `main.c` owns startup and the forever loop.
- `app.c` owns Blink timing and behavior.
- `bsp_led.c` owns PC13 GPIO and active-low polarity.
- `system_time.c` owns the SysTick time base.
- Only the required SPL modules are compiled: GPIO and RCC.
- Unsigned elapsed-time subtraction remains valid across 32-bit tick wraparound.

## References

Primary references:

- STMicroelectronics, STM32F103C8 product and datasheet: <https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html>
- STMicroelectronics, RM0008 reference manual: <https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf>
- STMicroelectronics, AN2606 system memory boot mode: <https://www.st.com/resource/en/application_note/cd00167594-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics.pdf>
- STMicroelectronics, ST-LINK/V2: <https://www.st.com/en/development-tools/st-link-v2.html>
- OpenOCD, debug adapter configuration: <https://openocd.org/doc/html/Debug-Adapter-Configuration.html>
- OpenOCD, Flash commands: <https://openocd.org/doc/html/Flash-Commands.html>
- OpenOCD, Flash programming: <https://openocd.org/doc/html/Flash-Programming.html>
