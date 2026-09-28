# ESP32-S3 port

Build, flash and run instructions for the ESP32-S3 target. The board is an
ESP32-S3-DevKitC-1. Hardware details are in [docs/esp32-s3.md](../../docs/esp32-s3.md).

The firmware at this stage (M0) enumerates as a vendor-class USB device. It has no audio
path and handles no bulk transfers. The firmware compiles with ESP-IDF v6.1 and has not
been tested on hardware.

## Requirements

- ESP-IDF v6.1.
- An ESP32-S3-DevKitC-1 board.
- Two USB cables.

On Linux, the user must be a member of the `dialout` group to open the serial port.

## Activate ESP-IDF

`idf.py` is not on `PATH` after installation. Run the activation script in each new
shell. The path depends on the installation method. For an ESP-IDF Installation Manager
installation:

```
. ~/.espressif/tools/activate_idf_v6.1.sh
```

For a manual installation, source `export.sh` in the ESP-IDF directory. The script sets
`IDF_PATH` and adds the toolchain to `PATH`.

## Build

Run from the repository root:

```
idf.py -C port/esp32s3 set-target esp32s3
idf.py -C port/esp32s3 build
```

`set-target` is required once. It creates `sdkconfig` from `sdkconfig.defaults`, and the
build directory `build/`. The firmware image is `build/cypherduck.bin`.

## Board connections

The board has two USB connectors:

| Connector | Function |
|---|---|
| USB-to-UART Port | Flashing and serial console, through UART0 |
| USB Port | Native USB, GPIO19 (D-) and GPIO20 (D+). The Cypherduck device. |

Connect both connectors to the host. Both connectors supply power.

## Flash and monitor

```
idf.py -C port/esp32s3 -p /dev/ttyUSB0 flash monitor
```

The serial device name depends on the USB-to-UART bridge chip. It is `/dev/ttyUSB0` or
`/dev/ttyACM0` on Linux. `ls /dev/ttyUSB* /dev/ttyACM*` lists the candidates.

`flash` writes the bootloader, the partition table and the application. The bridge
controls the EN and BOOT pins, so no button press is required.

`monitor` prints the console output of UART0. `Ctrl+]` exits the monitor.

If flashing fails to connect: hold BOOT, press and release RST, release BOOT, and retry.

## Check enumeration

The firmware starts TinyUSB after reset. The host detects the device on the USB Port
connector.

Linux:

```
lsusb -d 1209:0001
lsusb -v -d 1209:0001
```

Expected result:

- Vendor ID `0x1209`, product ID `0x0001`.
- Manufacturer `Cypherduck`, product `Cypherduck Audio Dongle`.
- Interface 0, class 255 (vendor specific), with bulk endpoints `0x01` OUT and `0x81` IN.

`sudo dmesg -w` prints the kernel messages during connection. No kernel driver claims
interface 0.

Windows binds WinUSB to the device without an INF file. Device Manager lists it under
"Universal Serial Bus devices".

## Files

| File | Content |
|---|---|
| `CMakeLists.txt` | ESP-IDF project root |
| `sdkconfig.defaults` | Project settings. Bluetooth disabled, `CONFIG_FREERTOS_HZ=1000`. |
| `main/main.c` | Entry point. Starts the USB PHY, TinyUSB and the TinyUSB task. |
| `main/tusb_config.h` | TinyUSB configuration for this target |
| `components/cd_core/` | Component wrapper for `core/` |
| `components/tinyusb/` | Component wrapper for `third_party/tinyusb/` |
