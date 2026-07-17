PROJECT ?= firmware
BUILD_DIR ?= build

PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
GDB := $(PREFIX)gdb

CPU_FLAGS := -mcpu=cortex-m3 -mthumb
DEFINES := -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER

INCLUDES := \
	-Iapp/inc \
	-Ibsp/inc \
	-Idrivers/inc \
	-Ilib/inc \
	-Imiddleware/inc \
	-Isystem/inc \
	-Ithird_party/CMSIS/CM3/CoreSupport \
	-Ithird_party/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
	-Ithird_party/STM32F10x_StdPeriph_Driver/inc

CFLAGS := $(CPU_FLAGS) $(DEFINES) $(INCLUDES) \
	-std=c11 -O0 -g3 \
	-Wall -Wextra -Wshadow -Wundef \
	-ffunction-sections -fdata-sections -fstack-usage \
	-MMD -MP

ASFLAGS := $(CPU_FLAGS) -x assembler-with-cpp -g3

LINKER_SCRIPT := linker/STM32F103C8Tx_FLASH.ld
LDFLAGS := $(CPU_FLAGS) \
	-T$(LINKER_SCRIPT) \
	-nostartfiles \
	-Wl,--gc-sections \
	-Wl,--print-memory-usage \
	-Wl,-Map=$(BUILD_DIR)/$(PROJECT).map

LDLIBS := -Wl,--start-group -lc -lm -lgcc -Wl,--end-group

APP_SOURCES := $(wildcard app/src/*.c)
BSP_SOURCES := $(wildcard bsp/src/*.c)
DRIVER_SOURCES := $(wildcard drivers/src/*.c)
LIB_SOURCES := $(wildcard lib/src/*.c)
MIDDLEWARE_SOURCES := $(wildcard middleware/src/*.c)
SYSTEM_SOURCES := $(wildcard system/src/*.c)
CMSIS_SOURCES := third_party/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c
SPL_SOURCES := $(wildcard third_party/STM32F10x_StdPeriph_Driver/src/*.c)

C_SOURCES := \
	$(APP_SOURCES) \
	$(BSP_SOURCES) \
	$(DRIVER_SOURCES) \
	$(LIB_SOURCES) \
	$(MIDDLEWARE_SOURCES) \
	$(SYSTEM_SOURCES) \
	$(CMSIS_SOURCES) \
	$(SPL_SOURCES)

ASM_SOURCES := system/startup/startup_stm32f10x_md_gcc.s

C_OBJECTS := $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
ASM_OBJECTS := $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.s=.o))
OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)
DEPENDENCIES := $(C_OBJECTS:.o=.d)

ELF := $(BUILD_DIR)/$(PROJECT).elf
HEX := $(BUILD_DIR)/$(PROJECT).hex
BIN := $(BUILD_DIR)/$(PROJECT).bin

.PHONY: all clean flash debug size help

all: $(ELF) $(HEX) $(BIN)

$(ELF): $(OBJECTS) $(LINKER_SCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	$(SIZE) $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary -S $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

size: $(ELF)
	$(SIZE) -A $<

flash: all
	./scripts/flash.sh $(ELF)

debug: all
	./scripts/debug.sh $(ELF)

clean:
	find $(BUILD_DIR) -mindepth 1 ! -name .gitkeep -delete

help:
	@echo "Targets:"
	@echo "  all    Build ELF, HEX, and BIN files"
	@echo "  flash  Build and flash with OpenOCD/ST-Link"
	@echo "  debug  Build and start an OpenOCD + GDB session"
	@echo "  size   Show section-level size information"
	@echo "  clean  Remove generated build files"
	@echo ""
	@echo "Variables:"
	@echo "  PROJECT=<name>       Output base name (default: firmware)"
	@echo "  PREFIX=<tool-prefix> Toolchain prefix (default: arm-none-eabi-)"

-include $(DEPENDENCIES)
