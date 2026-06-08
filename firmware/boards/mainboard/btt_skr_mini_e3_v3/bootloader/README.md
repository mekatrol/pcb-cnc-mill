# BTT SKR Mini E3 V3 Bootloader

This directory stores the upstream BIGTREETECH SKR Mini E3 V3.0 bootloader
region used to restore SD-card update behavior after direct-SWD recovery.

The copied source files were downloaded outside this repository to:

```text
/home/dad/Downloads/SKR-MINI-E3-V3.0-bootloader
```

The downloaded files were:

```text
SKR-MINI-E3-V3.0-bootloader.hex
SKR-MINI-E3-V3.0-bootloader.bin
```

The project stores the 8 KiB binary bootloader region:

```text
btt-skr-mini-e3-v3.0-bootloader-region-0x2000.bin
```

The BTT mainboard bootloader occupies `0x08000000` through `0x08001fff`. Normal
project firmware is linked at `0x08002000`, so restoring this bootloader-region
file does not overwrite the app region.

Restore the bootloader with ST-Link from the board build directory:

```sh
cd firmware/boards/mainboard/btt_skr_mini_e3_v3/build
make flash-btt-bootloader
```

Then flash this project's normal bootloader-offset firmware:

```sh
make flash-reset
```

After that, normal SD-card updates can use `firmware.bin` from the mainboard
build output directory.
