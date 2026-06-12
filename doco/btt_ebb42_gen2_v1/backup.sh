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
ls -lh "$OUT"
sha256sum "$OUT"
