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

BOARD_NAME ?= btt_tft35_e3
FLASH_LAYOUT ?= bootloader
FLASH_LAYOUT_SUFFIX := $(if $(filter direct,$(FLASH_LAYOUT)),-direct,)
TARGET     := pcb-cnc-mill-display-$(BOARD_NAME)$(FLASH_LAYOUT_SUFFIX)

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BOARD_DIR    := $(abspath $(MAKEFILE_DIR)..)
ROOT_DIR     := $(abspath $(BOARD_DIR)../../../../..)
FIRMWARE_DIR := $(ROOT_DIR)$(SEP)firmware
OBJ_DIR      := $(ROOT_DIR)$(SEP)firmware$(SEP)build$(SEP)display$(SEP)$(BOARD_NAME)$(FLASH_LAYOUT_SUFFIX)

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size
STFLASH := st-flash

ifeq ($(FLASH_LAYOUT),direct)
  FLASH_ADDR := 0x08000000
else
  FLASH_ADDR := 0x08003000
endif

CPUFLAGS := -mcpu=cortex-m3 -mthumb
CFLAGS   := $(CPUFLAGS) -std=c11 -Wall -Wextra -Werror -Os -ffreestanding -fdata-sections -ffunction-sections
ASFLAGS  := $(CPUFLAGS)

SRCS := \
	$(BOARD_DIR)$(SEP)display_hal.c \
	$(BOARD_DIR)$(SEP)startup.s

OBJS := $(patsubst $(BOARD_DIR)$(SEP)%,$(OBJ_DIR)$(SEP)%,$(SRCS:.c=.o))
OBJS := $(OBJS:.s=.o)

OUT_ELF := $(OBJ_DIR)$(SEP)$(TARGET).elf
OUT_MAP := $(OBJ_DIR)$(SEP)$(TARGET).map
OUT_BIN := $(OBJ_DIR)$(SEP)$(TARGET).bin

LDFLAGS := $(CPUFLAGS) -nostdlib -Wl,--gc-sections -Wl,-Map=$(OUT_MAP) -T$(MAKEFILE_DIR)linker.ld

.PHONY: all
all:
	"$(MAKE)" -C "$(FIRMWARE_DIR)" all BUILD_TARGET=display DISPLAY_BOARD_NAME=$(BOARD_NAME) BOARD_CLASS=display BOARD_NAME=$(BOARD_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT)

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
	$(STFLASH) --reset write $(abspath $(OUT_BIN)) $(FLASH_ADDR)

flash-reset: all
	@echo "[FLASH] Flashing binary with ST-Link"
	$(STFLASH) --connect-under-reset --reset write $(abspath $(OUT_BIN)) $(FLASH_ADDR)

.PHONY: flash-direct-reset
flash-direct-reset:
	"$(MAKE)" flash-reset FLASH_LAYOUT=direct

.PHONY: debug
debug: all
	@echo "[OPENOCD] Starting debug session"
	openocd -f interface/stlink.cfg -c "set CPUTAPID 0x1ba01477" -f target/stm32f2x.cfg \
		-c "debug_level 1" \
		-c "adapter speed 1800" \
		-c "init" \
		-c "halt" \
		-c "reset halt"

.PHONY: clean
clean:
	"$(MAKE)" -C "$(FIRMWARE_DIR)" clean BUILD_TARGET=display DISPLAY_BOARD_NAME=$(BOARD_NAME) BOARD_CLASS=display BOARD_NAME=$(BOARD_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT)
