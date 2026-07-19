PROJECT ?= firmware
BUILD_DIR ?= build

PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
SIZE := $(PREFIX)size
PYTHON ?= python3
OPENOCD ?= openocd

ARM_GDB := $(shell command -v $(PREFIX)gdb 2>/dev/null)
ifeq ($(ARM_GDB),)
GDB ?= gdb-multiarch
else
GDB ?= $(PREFIX)gdb
endif

CPU_FLAGS := -mcpu=cortex-m3 -mthumb
DEFINES := \
	-DSTM32F10X_MD \
	-DUSE_STDPERIPH_DRIVER \
	-DHSE_VALUE=8000000U

INCLUDES := \
	-Iapp/include \
	-Iservices/include \
	-Iecual/include \
	-Ibsp/bluepill/include \
	-Icommon/include \
	-Isystem \
	-Iplatform/include \
	-Iruntime/include \
	-Iconfig \
	-Ithird_party/CMSIS/CM3/CoreSupport \
	-Ithird_party/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
	-Ithird_party/STM32F10x_StdPeriph_Driver/inc

COMMON_FLAGS := \
	$(CPU_FLAGS) \
	$(DEFINES) \
	$(INCLUDES) \
	-ffreestanding \
	-fno-builtin \
	-ffunction-sections \
	-fdata-sections \
	-fno-common \
	-Wa,--noexecstack

CFLAGS := \
	$(COMMON_FLAGS) \
	-std=c11 \
	-Og \
	-g3 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wshadow \
	-Wundef \
	-Wdouble-promotion \
	-MMD \
	-MP

ASFLAGS := \
	$(COMMON_FLAGS) \
	-x assembler-with-cpp \
	-g3

LINKER_SCRIPT := linker/STM32F103C8T6.ld

LDFLAGS := \
	$(CPU_FLAGS) \
	-nostartfiles \
	-nostdlib \
	-T$(LINKER_SCRIPT) \
	-Wl,--gc-sections \
	-Wl,--build-id=none \
	-Wl,-z,noexecstack \
	-Wl,--print-memory-usage \
	-Wl,-Map=$(BUILD_DIR)/$(PROJECT).map

LDLIBS := -lgcc

-include config/modules.mk

PROJECT_SOURCE_DIRS := app services ecual bsp common system platform runtime
PROJECT_SOURCES := $(shell find $(PROJECT_SOURCE_DIRS) -type f -name '*.c' 2>/dev/null)

CMSIS_SOURCES := \
	third_party/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c

C_SOURCES := $(PROJECT_SOURCES) $(CMSIS_SOURCES) $(SPL_SOURCES)
ASM_SOURCES := startup/startup_stm32f10x_md.S

C_OBJECTS := $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
ASM_OBJECTS := $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.S=.o))
OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)
DEPENDENCIES := $(C_OBJECTS:.o=.d)

ELF := $(BUILD_DIR)/$(PROJECT).elf
HEX := $(BUILD_DIR)/$(PROJECT).hex
BIN := $(BUILD_DIR)/$(PROJECT).bin
LST := $(BUILD_DIR)/$(PROJECT).lst

.DEFAULT_GOAL := all

.PHONY: all clean check-layers check-tools check-gdb flash erase \
	debug-server debug size tree

all: check-layers check-tools $(ELF) $(HEX) $(BIN) $(LST)

check-layers:
	$(PYTHON) tools/scripts/check_layers.py

check-tools:
	@command -v $(CC) >/dev/null 2>&1 || { \
		echo "Error: $(CC) was not found."; \
		exit 1; \
	}
	@command -v $(OBJCOPY) >/dev/null 2>&1 || { \
		echo "Error: $(OBJCOPY) was not found."; \
		exit 1; \
	}

check-gdb:
	@command -v $(GDB) >/dev/null 2>&1 || { \
		echo "Error: debugger '$(GDB)' was not found."; \
		echo "Install arm-none-eabi-gdb or gdb-multiarch."; \
		exit 1; \
	}

$(ELF): $(OBJECTS) $(LINKER_SCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	$(SIZE) $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary -S $< $@

$(LST): $(ELF)
	$(OBJDUMP) -d -S $< > $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

flash: $(ELF)
	$(OPENOCD) \
		-f tools/openocd/bluepill_stlink.cfg \
		-c "program $(ELF) verify reset exit"

erase:
	$(OPENOCD) \
		-f tools/openocd/bluepill_stlink.cfg \
		-c "init" \
		-c "reset halt" \
		-c "stm32f1x mass_erase 0" \
		-c "reset run" \
		-c "shutdown"

debug-server:
	$(OPENOCD) -f tools/openocd/bluepill_stlink.cfg

debug: check-gdb $(ELF)
	$(GDB) -x tools/gdb/debug.gdb $(ELF)

size: $(ELF)
	$(SIZE) $(ELF)

tree:
	@find . \
		-path './.git' -prune -o \
		-path './build' -prune -o \
		-print | sort

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPENDENCIES)
