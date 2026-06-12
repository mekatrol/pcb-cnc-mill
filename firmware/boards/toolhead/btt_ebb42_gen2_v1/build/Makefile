ifeq ($(OS),Windows_NT)
  SEP := /
else
  SEP := /
endif

BOARD_NAME ?= btt_ebb42_gen2_v1
TARGET := pcb-cnc-mill-toolhead-$(BOARD_NAME)-direct

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BOARD_DIR    := $(abspath $(MAKEFILE_DIR)..)
ROOT_DIR     := $(abspath $(BOARD_DIR)../../../../..)
FIRMWARE_DIR := $(ROOT_DIR)$(SEP)firmware
OBJ_DIR      := $(FIRMWARE_DIR)$(SEP)build$(SEP)toolhead$(SEP)$(BOARD_NAME)-direct
OUT_BIN      := $(OBJ_DIR)$(SEP)$(TARGET).bin

DFU_UTIL          ?= dfu-util
DFU_DEVICE        ?= 0483:df11
DFU_ALT_SETTING   ?= 0
DFU_FLASH_ADDRESS ?= 0x08000000

.PHONY: all
all:
	"$(MAKE)" -C "$(FIRMWARE_DIR)" all BUILD_TARGET=toolhead TOOLHEAD_BOARD_NAME=$(BOARD_NAME) BOARD_CLASS=toolhead BOARD_NAME=$(BOARD_NAME) FLASH_LAYOUT=direct MCU=cortex-m0plus CHIP=STM32G0B1 DRIVER_NAMES=""

.PHONY: flash
flash: all
	@echo "[DFU] Flashing $(OUT_BIN) to $(DFU_DEVICE) at $(DFU_FLASH_ADDRESS)"
	$(DFU_UTIL) -d $(DFU_DEVICE) -a $(DFU_ALT_SETTING) -s $(DFU_FLASH_ADDRESS):leave -D "$(OUT_BIN)"

.PHONY: debug
debug:
	@echo "[DEBUG] DFU provides programming only; use an SWD probe for source debugging"
	@false

.PHONY: clean
clean:
	"$(MAKE)" -C "$(FIRMWARE_DIR)" clean BUILD_TARGET=toolhead TOOLHEAD_BOARD_NAME=$(BOARD_NAME) BOARD_CLASS=toolhead BOARD_NAME=$(BOARD_NAME) FLASH_LAYOUT=direct MCU=cortex-m0plus CHIP=STM32G0B1 DRIVER_NAMES=""
