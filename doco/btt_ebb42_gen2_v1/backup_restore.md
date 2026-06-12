# EBB42 Gen2 Firmware Backup and Restore

## Board Information

* Board: BTT EBB42 Gen2 V1.0
* MCU: STM32G0B1CBT6
* Flash size: 512 KiB
* DFU VID:PID: `0483:df11`

---

## Enter DFU Mode

1. Set USB/CAN switch to **USB**.
2. Hold **BOOT**.
3. Press and release **RST**.
4. Release **BOOT**.

Verify:

```bash
lsusb | grep 0483:df11
```

Expected:

```text
Bus XXX Device XXX: ID 0483:df11 STMicroelectronics STM Device in DFU Mode
```

---

## Backup Factory Firmware

Create `backup_ebb42.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

OUT="ebb42_factory_backup.bin"
PREFIX="ebb42_backup"
START=$((0x08000000))
CHUNK=$((0x8000))   # 32 KiB
COUNT=16            # 512 KiB total

rm -f ${PREFIX}_*.bin "$OUT"

for i in $(seq 0 $((COUNT - 1))); do
  addr=$(printf "0x%08X" $((START + i * CHUNK)))
  file=$(printf "%s_%02d.bin" "$PREFIX" "$i")

  echo "Reading $addr -> $file"

  sudo STM32_Programmer_CLI \
    -c port=USB1 \
    -u "$addr" "$CHUNK" "$file" || break

  test -s "$file" || break
done

cat ${PREFIX}_*.bin > "$OUT"

echo
echo "Backup complete:"
ls -lh "$OUT"

echo
echo "SHA256:"
sha256sum "$OUT"
```

Make executable:

```bash
chmod +x backup_ebb42.sh
```

Run:

```bash
./backup_ebb42.sh
```

Expected output:

```text
ebb42_factory_backup.bin
```

On this board only the first 128 KiB contained data. Reading beyond `0x08020000` returned an error.

Result:

```text
ebb42_factory_backup.bin
Size: 128 KiB
SHA256:
2811084963d3b197b3003bba46cc17c5b272ad2d16739dceb4eff4197208cbf7
```

---

## Save SHA256 Checksum

Generate checksum file:

```bash
sha256sum ebb42_factory_backup.bin > ebb42_factory_backup.bin.sha256
```

Verify later:

```bash
sha256sum -c ebb42_factory_backup.bin.sha256
```

Expected:

```text
ebb42_factory_backup.bin: OK
```

---

## Flash Custom Firmware

Place board into DFU mode.

Build the EBB42 Gen2 toolhead image:

```bash
make -C firmware toolhead TOOLHEAD_BOARD_NAME=btt_ebb42_gen2_v1
```

Flash with the board Makefile:

```bash
make -C firmware flash \
  BUILD_TARGET=toolhead \
  TOOLHEAD_BOARD_NAME=btt_ebb42_gen2_v1
```

If the current user cannot open the DFU device, keep `make` unprivileged and
run only `dfu-util` through `sudo`:

```bash
make -C firmware flash \
  BUILD_TARGET=toolhead \
  TOOLHEAD_BOARD_NAME=btt_ebb42_gen2_v1 \
  DFU_UTIL="sudo dfu-util"
```

The equivalent direct command is:

```bash
sudo dfu-util \
  -d 0483:df11 \
  -a 0 \
  -s 0x08000000:leave \
  -D firmware/build/toolhead/btt_ebb42_gen2_v1-direct/pcb-cnc-mill-toolhead-btt_ebb42_gen2_v1-direct.bin
```

The current smoke-test image uses TIM7 to toggle the onboard red `RLED` on
`PA8` every 1000 ms. One complete off/on blink cycle therefore takes two
seconds.

---

## Restore Factory Firmware

Place board into DFU mode.

Flash backup:

```bash
sudo dfu-util \
  -d 0483:df11 \
  -a 0 \
  -s 0x08000000:leave \
  -D ebb42_factory_backup.bin
```

Power-cycle the board.

---

## Verify DFU Device

```bash
dfu-util -l
```

Expected:

```text
Found DFU: [0483:df11]
```

or

```bash
sudo STM32_Programmer_CLI -l usb
```

Expected:

```text
Device Index : USB1
Device ID    : 0x0467
Device name  : STM32G0B0xx/B1xx/C1xx
```

---

## Useful Commands

Check board in DFU mode:

```bash
lsusb | grep 0483:df11
```

Check CubeProgrammer version:

```bash
STM32_Programmer_CLI --version
```

Check backup size:

```bash
ls -lh ebb42_factory_backup.bin
```

Check firmware checksum:

```bash
sha256sum firmware.bin
```
