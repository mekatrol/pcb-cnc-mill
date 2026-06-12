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
MAINBOARD_DISPLAY_NAME ?= none
MAINBOARD_DISPLAY_SUFFIX := $(if $(filter none,$(MAINBOARD_DISPLAY_NAME)),,-with-$(MAINBOARD_DISPLAY_NAME))
FLASH_LAYOUT ?= bootloader
FLASH_LAYOUT_SUFFIX := $(if $(filter direct,$(FLASH_LAYOUT)),-direct,)
TARGET     := pcb-cnc-mill-mainboard-$(BOARD_NAME)$(MAINBOARD_DISPLAY_SUFFIX)$(FLASH_LAYOUT_SUFFIX)

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BOARD_DIR    := $(abspath $(MAKEFILE_DIR)..)
ROOT_DIR     := $(abspath $(BOARD_DIR)../../../../..)
FIRMWARE_DIR := $(ROOT_DIR)$(SEP)firmware
OBJ_DIR      := $(ROOT_DIR)$(SEP)firmware$(SEP)build$(SEP)mainboard$(SEP)$(BOARD_NAME)$(MAINBOARD_DISPLAY_SUFFIX)
OBJ_DIR      := $(OBJ_DIR)$(FLASH_LAYOUT_SUFFIX)

OPENOCD := openocd
STLINK_SERIAL ?=
OPENOCD_STLINK_SERIAL_CMD := $(if $(STLINK_SERIAL),-c "adapter serial $(STLINK_SERIAL)",)
BTT_BOOTLOADER_BIN := $(BOARD_DIR)$(SEP)bootloader$(SEP)btt-skr-mini-e3-v3.0-bootloader-region-0x2000.bin
BTT_BOOTLOADER_ADDR := 0x08000000

OUT_BIN := $(OBJ_DIR)$(SEP)$(TARGET).bin

ifeq ($(FLASH_LAYOUT),direct)
  FLASH_ADDR := 0x08000000
else
  FLASH_ADDR := 0x08002000
endif

.PHONY: all
all:
	"$(MAKE)" -C "$(FIRMWARE_DIR)" all BUILD_TARGET=mainboard MAIN_BOARD_NAME=$(BOARD_NAME) BOARD_NAME=$(BOARD_NAME) MAINBOARD_DISPLAY_NAME=$(MAINBOARD_DISPLAY_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT)

.PHONY: flash
flash: all
	@echo "[FLASH] Flashing binary with ST-Link"
	$(OPENOCD) -f interface/stlink.cfg $(OPENOCD_STLINK_SERIAL_CMD) -f target/stm32g0x.cfg \
		-c "debug_level 1; adapter speed 1800; program $(abspath $(OUT_BIN)) verify reset exit $(FLASH_ADDR)"

.PHONY: flash-reset
flash-reset: all
	@echo "[FLASH] Flashing binary with ST-Link under reset"
	$(OPENOCD) -f interface/stlink.cfg $(OPENOCD_STLINK_SERIAL_CMD) -f target/stm32g0x.cfg \
		-c "debug_level 1" \
		-c "adapter speed 1800" \
		-c "reset_config srst_only srst_nogate connect_assert_srst" \
		-c "init" \
		-c "reset halt" \
		-c "program $(abspath $(OUT_BIN)) verify reset exit $(FLASH_ADDR)"

.PHONY: flash-direct-reset
flash-direct-reset:
	"$(MAKE)" flash-reset FLASH_LAYOUT=direct

.PHONY: flash-btt-bootloader
flash-btt-bootloader:
	@echo "[FLASH] Restoring BTT SKR Mini E3 V3 bootloader with ST-Link"
	$(OPENOCD) -f interface/stlink.cfg $(OPENOCD_STLINK_SERIAL_CMD) -f target/stm32g0x.cfg \
		-c "debug_level 1" \
		-c "adapter speed 1800" \
		-c "reset_config srst_only srst_nogate connect_assert_srst" \
		-c "init" \
		-c "reset halt" \
		-c "program $(abspath $(BTT_BOOTLOADER_BIN)) verify reset exit $(BTT_BOOTLOADER_ADDR)"

.PHONY: debug
debug: all
	@echo "[OPENOCD] Programming and halting"
	$(OPENOCD) -f interface/stlink.cfg $(OPENOCD_STLINK_SERIAL_CMD) -f target/stm32g0x.cfg \
		-c "debug_level 1" \
		-c "adapter speed 1800" \
		-c "init" \
		-c "halt" \
		-c "program {$(abspath $(OUT_ELF))} verify" \
		-c "reset halt"

.PHONY: clean
clean:
	"$(MAKE)" -C "$(FIRMWARE_DIR)" clean BUILD_TARGET=mainboard MAIN_BOARD_NAME=$(BOARD_NAME) BOARD_NAME=$(BOARD_NAME) MAINBOARD_DISPLAY_NAME=$(MAINBOARD_DISPLAY_NAME) FLASH_LAYOUT=$(FLASH_LAYOUT)
