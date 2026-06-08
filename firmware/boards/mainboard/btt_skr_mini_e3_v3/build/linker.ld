ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = IMAGE_FLASH_ORIGIN, LENGTH = IMAGE_FLASH_LENGTH
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 144K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector APP_FLASH_ORIGIN : {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
  } > FLASH

  .text : {
    . = ALIGN(4);
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  .data : {
    . = ALIGN(4);
    _sdata = .;
    _sidata = LOADADDR(.data);
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH

  .bss : {
    . = ALIGN(4);
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM
}
