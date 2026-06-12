# The STM32 system-memory DFU bootloader is in ROM, not in application flash.
# Custom firmware therefore owns the full 512 KiB application flash region.
DIRECT_APP_ORIGIN := 0x08000000
DIRECT_APP_LENGTH := 512K
