PROJECT          := 01-blink-led
BUILD_DIR        := build

TOOLCHAIN_PREFIX ?= arm-none-eabi-
CC               := $(TOOLCHAIN_PREFIX)gcc
AS               := $(TOOLCHAIN_PREFIX)gcc
OBJCOPY          := $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP          := $(TOOLCHAIN_PREFIX)objdump
SIZE             := $(TOOLCHAIN_PREFIX)size
GDB              := $(TOOLCHAIN_PREFIX)gdb
OPENOCD          ?= openocd
OPENOCD_CFG      ?= scripts/openocd.cfg

CPU_FLAGS        := -mcpu=cortex-m3 -mthumb
DEFINES          := -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=8000000U

INCLUDES         := \
	-Iapp/inc \
	-Ibsp/inc \
	-Isystem/inc \
	-Ithird_party/CMSIS/CM3/CoreSupport \
	-Ithird_party/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
	-Ithird_party/STM32F10x_StdPeriph_Driver/inc

C_SOURCES        := \
	app/src/main.c \
	app/src/app.c \
	bsp/src/bsp.c \
	bsp/src/bsp_led.c \
	system/src/system_time.c \
	system/src/stm32f10x_it.c \
	third_party/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c \
	third_party/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
	third_party/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c

ASM_SOURCES      := system/startup/startup_stm32f10x_md_gcc.s

C_OBJECTS        := $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
ASM_OBJECTS      := $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.s=.o))
OBJECTS          := $(C_OBJECTS) $(ASM_OBJECTS)
DEPS             := $(C_OBJECTS:.o=.d)

ELF              := $(BUILD_DIR)/$(PROJECT).elf
HEX              := $(BUILD_DIR)/$(PROJECT).hex
BIN              := $(BUILD_DIR)/$(PROJECT).bin
MAP              := $(BUILD_DIR)/$(PROJECT).map
LST              := $(BUILD_DIR)/$(PROJECT).lst
LDSCRIPT         := linker/STM32F103C8Tx_FLASH.ld

BUILD_TYPE       ?= debug
ifeq ($(BUILD_TYPE),release)
OPT_FLAGS        := -Os -g0
else
OPT_FLAGS        := -Og -g3
endif

COMMON_FLAGS     := $(CPU_FLAGS) $(DEFINES) $(INCLUDES) \
	-ffunction-sections -fdata-sections -fno-common

CFLAGS           := $(COMMON_FLAGS) $(OPT_FLAGS) -std=c11 \
	-Wall -Wextra -Wshadow -Wundef -Wdouble-promotion \
	-MMD -MP

ASFLAGS          := $(COMMON_FLAGS) $(OPT_FLAGS) -x assembler-with-cpp

LDFLAGS          := $(CPU_FLAGS) -T$(LDSCRIPT) \
	-nostartfiles -nostdlib \
	-Wl,-Map=$(MAP),--cref \
	-Wl,--gc-sections \
	-Wl,--print-memory-usage

LDLIBS           := -Wl,--start-group -lgcc -Wl,--end-group

OPENOCD_CMD      := $(OPENOCD) -f $(OPENOCD_CFG)

.PHONY: all build clean rebuild size probe flash erase reset openocd gdb disasm help

all: build size

build: $(ELF) $(HEX) $(BIN) $(LST)

$(ELF): $(OBJECTS) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary -S $< $@

$(LST): $(ELF)
	$(OBJDUMP) -h -S $< > $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

size: $(ELF)
	$(SIZE) -A -d $(ELF)

# Verify that OpenOCD can read and halt the target. For recovery with this
# four-wire probe, set BOOT0=1, BOOT1=0, then power-cycle/reset the board first.
probe:
	$(OPENOCD_CMD) \
		-c "init" \
		-c "halt" \
		-c "targets" \
		-c "shutdown"

# ST-LINK/V2 four-wire flash flow:
#   1. BOOT0=1, BOOT1=0; power-cycle/reset.
#   2. Connect, halt, erase/program, verify, then disconnect without reset.
#   3. Power off, set BOOT0=0, BOOT1=0, power on, and press RESET.
flash: $(ELF)
	@printf '%s\n' \
		"ST-LINK/V2 four-wire flash mode:" \
		"  1. Set BOOT0=1 and BOOT1=0." \
		"  2. Power-cycle or press RESET before this command." \
		""
	$(OPENOCD_CMD) \
		-c "init" \
		-c "halt" \
		-c "flash write_image erase $(abspath $(ELF))" \
		-c "verify_image $(abspath $(ELF))" \
		-c "shutdown"
	@printf '%s\n' \
		"" \
		"Flash verified." \
		"  1. Remove target power." \
		"  2. Set BOOT0=0 and BOOT1=0." \
		"  3. Restore power and press RESET to run the application."

erase:
	@printf '%s\n' \
		"Set BOOT0=1, BOOT1=0 and power-cycle/reset before mass erase."
	$(OPENOCD_CMD) \
		-c "init" \
		-c "halt" \
		-c "stm32f1x mass_erase 0" \
		-c "shutdown"

# A four-wire ST-LINK has no NRST signal, so reset is a manual board action.
reset:
	@printf '%s\n' \
		"Hardware reset is unavailable through this four-wire ST-LINK." \
		"Press the Blue Pill RESET button or power-cycle the board."

openocd:
	$(OPENOCD_CMD)

gdb: $(ELF)
	$(GDB) $(ELF) \
		-ex "target extended-remote localhost:3333" \
		-ex "monitor halt"

disasm: $(LST)
	@printf '%s\n' "Generated: $(LST)"

clean:
	rm -rf $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)
	touch $(BUILD_DIR)/.gitignore

rebuild: clean all

help:
	@printf '%s\n' \
		"make                      Build debug firmware" \
		"make BUILD_TYPE=release   Build size-optimized firmware" \
		"make size                 Show section sizes" \
		"make probe                Test ST-LINK/SWD target access" \
		"make flash                Build, erase, program, and verify" \
		"make erase                Mass-erase internal Flash" \
		"make reset                Show manual reset instructions" \
		"make openocd              Start an OpenOCD GDB server" \
		"make gdb                  Connect GDB and halt the target" \
		"make clean                Remove generated files"

-include $(DEPS)
