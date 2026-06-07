# OS-specific commands and separator
ifeq ($(OS),Windows_NT)
  DELCMD = del /Q
  MKDIR  = if not exist "$(dir $@)" mkdir "$(dir $@)"
  RM     = rmdir /S /Q
  SEP    = /
else
  DELCMD = rm -rf
  MKDIR  = mkdir -p $(dir $@)
  RM     = rm -rf
  SEP    = /
endif

BOARD_NAME ?= btt_skr_mini_e3_v3
TARGET     := pcb-cnc-mill-mainboard-$(BOARD_NAME)

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BOARD_DIR    := $(abspath $(MAKEFILE_DIR)..)
ROOT_DIR     := $(abspath $(BOARD_DIR)../../../../..)
FIRMWARE_DIR := $(ROOT_DIR)$(SEP)firmware
OBJ_DIR      := $(ROOT_DIR)$(SEP)firmware$(SEP)build$(SEP)mainboard$(SEP)$(BOARD_NAME)

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CPUFLAGS := -mcpu=cortex-m0plus -mthumb
CFLAGS   := $(CPUFLAGS) -std=c11 -Wall -Wextra -Werror -Os -ffreestanding -fdata-sections -ffunction-sections
ASFLAGS  := $(CPUFLAGS)

SRCS := \
	$(BOARD_DIR)$(SEP)main.c \
	$(BOARD_DIR)$(SEP)startup.s

OBJS := $(patsubst $(BOARD_DIR)$(SEP)%,$(OBJ_DIR)$(SEP)%,$(SRCS:.c=.o))
OBJS := $(OBJS:.s=.o)

OUT_ELF := $(OBJ_DIR)$(SEP)$(TARGET).elf
OUT_MAP := $(OBJ_DIR)$(SEP)$(TARGET).map
OUT_BIN := $(OBJ_DIR)$(SEP)$(TARGET).bin

LDFLAGS := $(CPUFLAGS) -nostdlib -Wl,--gc-sections -Wl,-Map=$(OUT_MAP) -T$(MAKEFILE_DIR)linker.ld

.PHONY: all
all:
	"$(MAKE)" -C "$(FIRMWARE_DIR)" all BUILD_TARGET=mainboard MAIN_BOARD_NAME=$(BOARD_NAME) BOARD_NAME=$(BOARD_NAME)

$(OUT_BIN): $(OUT_ELF)
	$(OBJCOPY) -O binary $< $@

$(OUT_ELF): $(OBJS) $(MAKEFILE_DIR)linker.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OBJ_DIR)$(SEP)%.o: $(BOARD_DIR)$(SEP)%.c
	$(MKDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)$(SEP)%.o: $(BOARD_DIR)$(SEP)%.s
	$(MKDIR)
	$(CC) $(ASFLAGS) -c $< -o $@

.PHONY: flash
flash: all
	@echo "[FLASH] Flashing binary with ST-Link"
	openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
		-c "debug_level 1; adapter speed 1800; program $(abspath $(OUT_BIN)) verify reset exit 0x08000000"

.PHONY: debug
debug: all
	@echo "[OPENOCD] Programming and halting"
	openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
		-c "debug_level 1" \
		-c "adapter speed 1800" \
		-c "init" \
		-c "halt" \
		-c "program {$(abspath $(OUT_ELF))} verify" \
		-c "reset halt"

.PHONY: clean
clean:
	"$(MAKE)" -C "$(FIRMWARE_DIR)" clean BUILD_TARGET=mainboard MAIN_BOARD_NAME=$(BOARD_NAME) BOARD_NAME=$(BOARD_NAME)
