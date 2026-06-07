# Upstream Firmware References

## BIGTREETECH TouchScreenFirmware

Use the upstream BIGTREETECH firmware as the reference implementation for BTT
TFT display board bring-up, especially LCD controller probing, FSMC/EXMC LCD
bus setup, XPT2046 touch handling, encoder pins, buzzer pins, and knob LED
behavior.

Expected local relative path from this repository:

```text
../BIGTREETECH-TouchScreenFirmware
```

Clone it from this repository's parent directory if it is missing:

```sh
cd ..
git clone https://github.com/bigtreetech/BIGTREETECH-TouchScreenFirmware.git
```

Useful upstream files for the BTT TFT35 E3/GD TFT35 E3 display target:

- `../BIGTREETECH-TouchScreenFirmware/platformio.ini`
- `../BIGTREETECH-TouchScreenFirmware/TFT/src/User/Variants/pin_TFT35_E3_V3_0.h`
- `../BIGTREETECH-TouchScreenFirmware/TFT/src/User/Variants/pin_GD_TFT35_E3_V3_0.h`
- `../BIGTREETECH-TouchScreenFirmware/TFT/src/User/Variants/pin_TFT35_V3_0.h`
- `../BIGTREETECH-TouchScreenFirmware/TFT/src/User/Variants/pin_GD_TFT35_V3_0.h`
- `../BIGTREETECH-TouchScreenFirmware/TFT/src/User/Hal/LCD_Init.c`
- `../BIGTREETECH-TouchScreenFirmware/TFT/src/User/Hal/gd32f20x/lcd.c`
- `../BIGTREETECH-TouchScreenFirmware/TFT/src/User/Hal/gd32f20x/lcd.h`
- `../BIGTREETECH-TouchScreenFirmware/TFT/src/User/Hal/xpt2046.c`
- `../BIGTREETECH-TouchScreenFirmware/TFT/src/User/Hal/Knob_LED.c`
