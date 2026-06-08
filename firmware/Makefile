# ==========================================================
# Project configuration (defaults; CLI can override)
# ==========================================================
BUILD_TARGET       ?= mainboard
MAIN_BOARD_NAME    ?= btt_skr_mini_e3_v3
DISPLAY_BOARD_NAME ?= btt_tft35_e3
MAINBOARD_DISPLAY_NAME ?= none
FLASH_LAYOUT       ?= bootloader
DRIVER_NAMES       ?=

ifeq ($(BUILD_TARGET),mainboard)
  BOARD_CLASS ?= mainboard
  BOARD_NAME  ?= $(MAIN_BOARD_NAME)
  MCU         ?= cortex-m0plus
  CHIP        ?= STM32G0B1
  ENTRY_CSRCS := core/mainboard/main.c
  ifneq ($(MAINBOARD_DISPLAY_NAME),none)
    MAINBOARD_DISPLAY_ENABLED := 1
    ifeq ($(wildcard boards/display_module/$(MAINBOARD_DISPLAY_NAME)),)
      $(error Unknown MAINBOARD_DISPLAY_NAME "$(MAINBOARD_DISPLAY_NAME)"; expected a folder under boards/display_module or use none)
    endif
  endif
else ifeq ($(BUILD_TARGET),display)
  BOARD_CLASS ?= display
  BOARD_NAME  ?= $(DISPLAY_BOARD_NAME)
  MCU         ?= cortex-m3
  CHIP        ?= GD32F205
  ENTRY_CSRCS := core/display/main.c
else
  $(error Unknown BUILD_TARGET "$(BUILD_TARGET)"; use mainboard or display)
endif

export BUILD_TARGET MAIN_BOARD_NAME DISPLAY_BOARD_NAME MAINBOARD_DISPLAY_NAME FLASH_LAYOUT BOARD_CLASS BOARD_NAME MCU CHIP DRIVER_NAMES

# ====================================
# OS-specific shell/commands
# ====================================
ifeq ($(OS),Windows_NT)
  SHELL := cmd
  .SHELLFLAGS := /c
  DELCMD = del /Q
  MKDIR  = if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
  RM     = rmdir /S /Q
  SEP    = /
else
  DELCMD = rm -f
  MKDIR  = mkdir -p $(dir $@)
  RM     = rm -rf
  SEP    = /
endif

# =========================
# Toolchain
# =========================
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

# =========================
# Directories
# =========================
SRC_DIR         := .
CORE_DIR        := core
HAL_INCLUDE_DIR := hal/include
BOARD_DIR       := boards/$(BOARD_CLASS)/$(BOARD_NAME)
BOARD_BLD_DIR   := $(BOARD_DIR)/build
MAINBOARD_DISPLAY_DIR := boards/display_module/$(MAINBOARD_DISPLAY_NAME)
DRIVER_DIRS     := $(foreach name,$(DRIVER_NAMES),drivers/$(name))
MAINBOARD_DISPLAY_OBJ_SUFFIX := $(if $(MAINBOARD_DISPLAY_ENABLED),-with-$(MAINBOARD_DISPLAY_NAME),)
FLASH_LAYOUT_OBJ_SUFFIX := $(if $(filter direct,$(FLASH_LAYOUT)),-direct,)
OBJ_DIR         := build/$(BUILD_TARGET)/$(BOARD_NAME)$(MAINBOARD_DISPLAY_OBJ_SUFFIX)$(FLASH_LAYOUT_OBJ_SUFFIX)

-include $(BOARD_BLD_DIR)$(SEP)bootloader.mk

ifeq ($(FLASH_LAYOUT),direct)
  APP_FLASH_ORIGIN ?= $(DIRECT_APP_ORIGIN)
  APP_FLASH_LENGTH ?= $(DIRECT_APP_LENGTH)
  BOOTLOADER_UPDATE_BIN_NAME :=
else ifeq ($(FLASH_LAYOUT),bootloader)
  APP_FLASH_ORIGIN ?= $(BOOTLOADER_APP_ORIGIN)
  APP_FLASH_LENGTH ?= $(BOOTLOADER_APP_LENGTH)
else
  $(error Unknown FLASH_LAYOUT "$(FLASH_LAYOUT)"; use bootloader or direct)
endif

APP_FLASH_ORIGIN ?= 0x08000000
APP_FLASH_LENGTH ?= 256K

# =========================
# Outputs
# =========================
TARGET  := pcb-cnc-mill-$(BUILD_TARGET)-$(BOARD_NAME)$(MAINBOARD_DISPLAY_OBJ_SUFFIX)$(FLASH_LAYOUT_OBJ_SUFFIX)
OUT_ELF := $(OBJ_DIR)$(SEP)$(TARGET).elf
OUT_MAP := $(OBJ_DIR)$(SEP)$(TARGET).map
OUT_BIN := $(OBJ_DIR)$(SEP)$(TARGET).bin
OUT_BOOTLOADER_UPDATE_BIN := $(if $(BOOTLOADER_UPDATE_BIN_NAME),$(OBJ_DIR)$(SEP)$(BOOTLOADER_UPDATE_BIN_NAME),)

# =========================
# Flags
# =========================
CPUFLAGS := -mcpu=$(MCU) -mthumb
CFLAGS   := $(CPUFLAGS) -std=c11 -Wall -Wextra -Werror -Os -ffreestanding -fdata-sections -ffunction-sections
ASFLAGS  := $(CPUFLAGS)

LD_SCRIPT := $(BOARD_BLD_DIR)$(SEP)linker.ld
LDFLAGS   := $(CPUFLAGS) -nostdlib -Wl,--gc-sections -Wl,-Map=$(OUT_MAP) \
             -Wl,--defsym=APP_FLASH_ORIGIN=$(APP_FLASH_ORIGIN) \
             -Wl,--defsym=APP_FLASH_LENGTH=$(APP_FLASH_LENGTH) \
             -T$(LD_SCRIPT)

CFLAGS += -I$(CORE_DIR) -I$(CORE_DIR)/$(BUILD_TARGET) -I$(HAL_INCLUDE_DIR) -I$(BOARD_DIR)
CFLAGS += $(foreach dir,$(DRIVER_DIRS),-I$(dir) -I$(dir)/include)
ifneq ($(MAINBOARD_DISPLAY_ENABLED),)
  CFLAGS += -DMAINBOARD_HAS_DISPLAY_MODULE=1 -I$(CORE_DIR)/display -I$(MAINBOARD_DISPLAY_DIR)
endif

# =========================
# Sources
# =========================
COMMON_CSRCS := core/runtime/scheduler.c \
                core/runtime/chirp.c

CSRCS := $(ENTRY_CSRCS) \
         $(COMMON_CSRCS) \
         $(wildcard $(BOARD_DIR)/*.c) \
         $(if $(MAINBOARD_DISPLAY_ENABLED),$(wildcard $(MAINBOARD_DISPLAY_DIR)/*.c)) \
         $(foreach dir,$(DRIVER_DIRS),$(wildcard $(dir)/*.c))

ASRCS := $(wildcard $(BOARD_DIR)/*.s) \
         $(if $(MAINBOARD_DISPLAY_ENABLED),$(wildcard $(MAINBOARD_DISPLAY_DIR)/*.s)) \
         $(foreach dir,$(DRIVER_DIRS),$(wildcard $(dir)/*.s))

C_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(CSRCS))
A_OBJS := $(patsubst %.s,$(OBJ_DIR)/%.o,$(ASRCS))
OBJS   := $(A_OBJS) $(C_OBJS)

# =========================
# Default target
# =========================
.PHONY: all
all: $(OUT_BIN) $(OUT_BOOTLOADER_UPDATE_BIN)
	$(SIZE) $(OUT_ELF)

$(OUT_BIN): $(OUT_ELF)
	$(OBJCOPY) -O binary $< $@

$(OUT_BOOTLOADER_UPDATE_BIN): $(OUT_ELF)
	$(OBJCOPY) -O binary $< $@

$(OUT_ELF): $(OBJS) $(LD_SCRIPT)
	@echo [LD] Linking $@
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: %.c
	@echo [CC] $<
	@$(MKDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.s
	@echo [AS] $<
	@$(MKDIR)
	$(CC) $(ASFLAGS) -c $< -o $@

# =========================
# Build selection helpers
# =========================
.PHONY: mainboard
mainboard:
	"$(MAKE)" build BUILD_TARGET=mainboard BOARD_CLASS=mainboard BOARD_NAME=$(MAIN_BOARD_NAME) MAIN_BOARD_NAME=$(MAIN_BOARD_NAME) MAINBOARD_DISPLAY_NAME=$(MAINBOARD_DISPLAY_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT) MCU=cortex-m0plus CHIP=STM32G0B1 DRIVER_NAMES="$(DRIVER_NAMES)"

.PHONY: display
display:
	"$(MAKE)" build BUILD_TARGET=display BOARD_CLASS=display BOARD_NAME=$(DISPLAY_BOARD_NAME) DISPLAY_BOARD_NAME=$(DISPLAY_BOARD_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT) MCU=cortex-m3 CHIP=GD32F205 DRIVER_NAMES="$(DRIVER_NAMES)"

.PHONY: build
build:
	"$(MAKE)" clean BUILD_TARGET=$(BUILD_TARGET) BOARD_CLASS=$(BOARD_CLASS) BOARD_NAME=$(BOARD_NAME) MAINBOARD_DISPLAY_NAME=$(MAINBOARD_DISPLAY_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT) MCU=$(MCU) CHIP=$(CHIP)
	"$(MAKE)" all BUILD_TARGET=$(BUILD_TARGET) BOARD_CLASS=$(BOARD_CLASS) BOARD_NAME=$(BOARD_NAME) MAINBOARD_DISPLAY_NAME=$(MAINBOARD_DISPLAY_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT) MCU=$(MCU) CHIP=$(CHIP) DRIVER_NAMES="$(DRIVER_NAMES)"

# ===============================================
# Flash / Debug (delegated to board build dir)
# ===============================================
.PHONY: flash
flash:
	@echo [FLASH] $(CHIP) $(BUILD_TARGET) board $(BOARD_NAME)
	"$(MAKE)" -C $(BOARD_BLD_DIR) flash BUILD_TARGET=$(BUILD_TARGET) MAIN_BOARD_NAME=$(MAIN_BOARD_NAME) DISPLAY_BOARD_NAME=$(DISPLAY_BOARD_NAME) MAINBOARD_DISPLAY_NAME=$(MAINBOARD_DISPLAY_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT) BOARD_NAME=$(BOARD_NAME) DRIVER_NAMES="$(DRIVER_NAMES)"

.PHONY: debug
debug:
	@echo [DEBUG] $(CHIP) $(BUILD_TARGET) board $(BOARD_NAME)
	"$(MAKE)" -C $(BOARD_BLD_DIR) debug BUILD_TARGET=$(BUILD_TARGET) MAIN_BOARD_NAME=$(MAIN_BOARD_NAME) DISPLAY_BOARD_NAME=$(DISPLAY_BOARD_NAME) MAINBOARD_DISPLAY_NAME=$(MAINBOARD_DISPLAY_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT) BOARD_NAME=$(BOARD_NAME) DRIVER_NAMES="$(DRIVER_NAMES)"

# =========================
# Clean
# =========================
.PHONY: clean
clean:
	@echo [CLEAN] Removing $(OBJ_DIR)
	-$(RM) "$(OBJ_DIR)"

.PHONY: cleanall
cleanall:
	@echo [CLEAN] Removing all firmware build artifacts
	-$(RM) build

# =========================
# Config echo
# =========================
.PHONY: print-config
print-config:
	@echo BUILD_TARGET=$(BUILD_TARGET)
	@echo MAIN_BOARD_NAME=$(MAIN_BOARD_NAME)
	@echo DISPLAY_BOARD_NAME=$(DISPLAY_BOARD_NAME)
	@echo MAINBOARD_DISPLAY_NAME=$(MAINBOARD_DISPLAY_NAME)
	@echo FLASH_LAYOUT=$(FLASH_LAYOUT)
	@echo BOARD_CLASS=$(BOARD_CLASS)
	@echo BOARD_NAME=$(BOARD_NAME)
	@echo MCU=$(MCU)
	@echo CHIP=$(CHIP)
	@echo DRIVER_NAMES=$(DRIVER_NAMES)
	@echo BOARD_DIR=$(BOARD_DIR)
	@echo APP_FLASH_ORIGIN=$(APP_FLASH_ORIGIN)
	@echo APP_FLASH_LENGTH=$(APP_FLASH_LENGTH)
	@echo OUT_BIN=$(OUT_BIN)
	@echo OUT_BOOTLOADER_UPDATE_BIN=$(OUT_BOOTLOADER_UPDATE_BIN)
