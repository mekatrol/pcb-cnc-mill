# BTT GD TFT35 Bootloader

This directory stores the upstream BIGTREETECH GD TFT35 V3.0.1 bootloader
binary used to restore SD-card firmware update behavior.

Source repository:

```text
https://github.com/bigtreetech/BIGTREETECH-TouchScreenFirmware.git
```

Source path:

```text
Bootloaders/GD TFT35 V3.0.1 Bootloader fix/bootloader tft35 GD.bin
```

Upstream commit:

```text
01f15ea2 Bootloader for BTT GD TFT35 (#2657)
```

Local filename:

```text
btt-gd-tft35-v3.0.1-bootloader.bin
```

SHA-256:

```text
b28ab4342030e360876217d599ab2c1b2cee0024c4e335f3cac8851d1fa4346c
```

The upstream binary is a full 256 KiB flash image. It contains the BTT
bootloader in `0x08000000` through `0x08002fff`, and it also contains an
upstream BTT application image starting at `0x08003000`. The project flash
target uses this extracted bootloader-region file so restoring the bootloader
does not write into this project's application region:

```text
btt-gd-tft35-v3.0.1-bootloader-region-0x3000.bin
```

Bootloader-region SHA-256:

```text
7db7fd762cab63469caee913f4239ac099f7fab01372ea40dc52ba3a25d2a84a
```

Restore it from `firmware/boards/display/btt_tft35_e3/build`:

```sh
make flash-btt-bootloader
make flash-reset
```
