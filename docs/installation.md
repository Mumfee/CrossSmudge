---
title: Installation
nav_order: 2
---

# Installation

## Supported Devices

- Xteink X3, X4
- Xteink X4 Classic and X4 Pro
- Seeed Studio Sticky

Don't have a device yet? Get one directly from [Xteink](https://go.sjv.io/X4RGBb) or [Seeed Studio](https://www.seeedstudio.com/reTerminal-Sticky-p-6861.html?sensecap_affiliate=1Nxo3Gw&referring_service=link).

Note: Your purchases using the above affiliate links help support ongoing development of Crossink.

## Web Installation via USB

#### For new installs and updates.

1. Download the `firmware-*.bin` file for your device from the [CrossSmudge Releases](https://github.com/Mumfee/CrossSmudge/releases/latest).
2. Connect your device (Xteink X3, X4, X4 Pro, Sticky) to your computer via USB-C and ensure it is powered on.
3. Navigate to [https://crosspointreader.com/#flash-tools](https://crosspointreader.com/#flash-tools) in a WebSerial-compatible browser (Chrome, Edge, Brave, Opera).
4. Select your device model from the list.
5. Under firmware selection, choose **"Custom .bin"** (*Upload file*).
6. Select the downloaded `firmware-*.bin` file and click **Flash**.

Keep the reader connected during the download-mode and flashing steps.

## USB Drive

On X4 Pro, choose `Home > File Transfer > USB Drive` to expose the SD card to
your computer. Eject the drive from the computer before disconnecting it; the
reader restarts to Home when the drive is safely ejected or the cable is
removed.

## SD Card Firmware Update

#### For installing newer versions of CrossSmudge. Can be used by USB locked devices.

1. Download the `firmware-*.bin` file from the [CrossSmudge Releases](https://github.com/Mumfee/CrossSmudge/releases/latest).
2. Place the downloaded `firmware-*.bin` file on your SD card. You can place this file anywhere.
3. On your device, go to `Settings > System > SD Card Firmware Update` and navigate to the `.bin` file and update.

## USB Locked Devices

If your device has USB data transfer disabled:

1. Download the `firmware-*.bin` file from the [CrossSmudge Releases](https://github.com/Mumfee/CrossSmudge/releases/latest).
2. Copy the `.bin` file to the root of your SD card using an SD card reader.
3. Insert the card into your device, go to `Settings > System > SD Card Firmware Update`, and select the `.bin` file.

## Command Line

These instructions are for macOS and Linux. Windows users should use the web installer.

Install `esptool`:

```sh
pip3 install esptool
```

Download the `firmware-*.bin` file from the [CrossSmudge Releases](https://github.com/Mumfee/CrossSmudge/releases/latest), then connect your device with USB-C.

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
