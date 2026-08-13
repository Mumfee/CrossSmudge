---
title: Installation
nav_order: 2
---

# Installation

## Supported Devices

- Xteink X3, X4
- Seeed Studio Sticky

## Web Installation via USB

#### For new installs and updates.

1. Navigate to [https://inky.crosssmudge.dev/#flash-tools](https://inky.crosssmudge.dev/#flash-tools) and select your device model.
2. The latest version will be automatically selected, but if you ever want to revert to an earlier build, you can select it from the dropdown.
3. Choose the firmware option you want to install.
4. Click on the "Flash Firmware" button

## SD Card Firmware Update

#### For installing newer versions of CrossSmudge. Can be used by USB locked devices.

1. Follow the same steps from the Web Installation method above. There will be an option to download the firmware instead of USB flashing.
2. Place the downloaded `firmware-*.bin` file on your SD card. You can place this file anywhere.
3. Go to `Settings > System > SD Card Firmware Update` and navigate to the `.bin` file and update.

## USB Locked Devices

If your device has USB data transfer disabled:

1. Navigate to [https://inky.crosssmudge.dev/#flash-tools](https://inky.crosssmudge.dev/#flash-tools) and check the box for "I have a locked device" at the top.
2. The latest version will be automatically selected, but if you ever want to revert to an earlier build, you can select it from the dropdown.
3. Choose the firmware option you want to download.
4. Click on the "Download update.bin" button and follow the instructions.

## Command Line

These instructions are for macOS and Linux. Windows users should use the web installer.

Install `esptool`:

```sh
pip3 install esptool
```

Download the `firmware-*.bin` file from the [releases page](https://github.com/uxjulia/CrossSmudge/releases), then connect your device with USB-C.

Find the device port:

```sh
# Linux
dmesg | grep tty

# macOS
ls /dev/cu.*
```

Flash the firmware:

```sh
# Linux
esptool.py --chip esp32c3 --port /dev/ttyACM0 --baud 921600 write_flash 0x10000 /path/to/firmware.bin

# macOS
esptool.py --chip esp32c3 --port /dev/cu.usbmodem2101 --baud 921600 write_flash 0x10000 /path/to/firmware.bin
```

Replace the port and firmware path with your actual values.
