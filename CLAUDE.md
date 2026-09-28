# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Cypherduck is firmware for a USB audio dongle. A source encrypts audio frames, a PC relay
forwards them over USB bulk transfers, and the device decrypts them and drives an I2S DAC
(PCM5102A). The PC relay holds no session key.

- Prototyping target: ESP32-S3 (milestones M0 to M4).
- Production target: STM32U5, board B-U585I-IOT02A (milestones M5 to M7).

Current state: the USB descriptors and TinyUSB integration exist (M0 in progress).
`protocol.c`, `jitter.c`, `crypto.c`, `cd_protocol.h` and `cd_sink_port.h` are empty stubs.
`port/stm32/` and `host/` hold only `.gitkeep`. The ESP32-S3 build compiles with ESP-IDF
v6.1 and has not been tested on hardware. See `docs/milestones.md` for the order of work and the exit criteria of each
milestone.

## Build

ESP32-S3, with ESP-IDF (`IDF_PATH` must be set):

```
idf.py -C port/esp32s3 set-target esp32s3
idf.py -C port/esp32s3 build
idf.py -C port/esp32s3 -p <port> flash monitor
```

ESP-IDF compiles with `-Werror`. TinyUSB emits `#warning` for renamed `CFG_TUD_*` options, so
a renamed option in `tusb_config.h` stops the build.

A static library member that only TinyUSB references is not extracted by the linker, and
TinyUSB's weak defaults are used instead. The `cd_core` component is therefore linked with
`WHOLE_ARCHIVE`. The STM32 build requires the same treatment for `cd_core`.

STM32, with plain CMake from the repository root:

```
cmake -B build -DCD_PORT=stm32
cmake --build build
```

The STM32 configure step fails with `FATAL_ERROR`. It requires `port/stm32/target.cmake`,
which does not exist.

There are no tests and no lint configuration.

## Architecture

### Portability boundary

TinyUSB is the portability boundary. Code in `core/` contains no MCU-specific code and no
MCU conditionals. A target is selected by linking a different port. Keep it that way: put
chip-specific code in `port/<name>/`.

- `core/` - portable firmware. Supplies the USB descriptors and the class 0xFF callbacks.
  Public headers are in `core/include/cd/`.
- `port/<name>/` - per-chip code: entry point, PHY setup, `tusb_config.h`.
- `third_party/tinyusb/` - TinyUSB 0.18.0, `src/` only, copied as plain files (not a
  submodule). Do not edit it. The upgrade procedure is in `docs/tinyusb.md` and
  `third_party/tinyusb.VERSION`.
- `host/` - planned host library (`libcypherduck`) and CLI tool (`cd-send`).

### Two build systems, one source list

Each shared tree has its source list in one `.cmake` file, read by both builds:

- `core/sources.cmake` -> `CD_CORE_SOURCES`, `CD_CORE_INCLUDE`
- `third_party/tinyusb_sources.cmake` -> `CD_TINYUSB_SOURCES`, `CD_TINYUSB_INCLUDE`

The plain CMake build (root `CMakeLists.txt`, `core/CMakeLists.txt`,
`third_party/CMakeLists.txt`) includes these files. The ESP-IDF project root is
`port/esp32s3/`, not the repository root. Its `components/cd_core/` and
`components/tinyusb/` are wrappers that include the same lists and call
`idf_component_register`. The shared trees contain no ESP-IDF files.

When adding a source file to `core/`, add it to `core/sources.cmake` only.

A port for the plain CMake build provides `port/<name>/target.cmake`, which must set
`CD_TUSB_MCU`, `CD_TUSB_DCD_SOURCES` and `CD_TUSB_CONFIG_DIR`. The ESP-IDF wrapper sets the
DCD sources (Synopsys DWC2) and `CFG_TUSB_MCU=OPT_MCU_ESP32S3` itself. Both targets use a
DWC2 USB core.

### USB device

- `core/src/descriptors.c` holds the descriptors as constant byte arrays and exposes
  accessors declared in `cd/cd_usb_port.h`. It includes no TinyUSB header.
- `core/src/usb_callbacks.c` implements the TinyUSB `tud_*_cb` callbacks by calling those
  accessors. It converts ASCII strings to UTF-16LE and serves the Microsoft OS 2.0
  descriptor set on the vendor request with `wIndex == 7`.
- VID/PID `0x1209`/`0x0001` (pid.codes). Interface 0 is class 0xFF with bulk OUT `0x01`
  (encrypted frames) and bulk IN `0x81` (status). A future audio class function is
  appended as interfaces 1 and 2; interface 0 does not change.
- Windows binds WinUSB through the BOS and MS OS 2.0 descriptors, without an INF file.
- `CFG_TUD_ENDPOINT0_SIZE` in each port's `tusb_config.h` must match `bMaxPacketSize0` in
  `descriptors.c`. A frame is 344 bytes.

### ESP32-S3 hardware notes

- The device uses the native USB OTG port (GPIO19 D-, GPIO20 D+). USB Serial/JTAG shares
  the transceiver and is unavailable. The console runs over UART0 through the
  USB-to-UART bridge port.
- `sdkconfig.defaults` disables Bluetooth and sets `CONFIG_FREERTOS_HZ=1000` for
  `tud_task()`.

## Documentation

`docs/` holds the design: data path and frame order (`datapath.md`), handshake and key
exchange, eFuse and boot on ESP32-S3, STM32U5 key storage. The design plan referenced as
"section N of the design plan" is linked from `docs/milestones.md`.

Docs and source comments follow the `readable-tech-docs` skill in `.claude/skills/`: short,
plain, precise sentences. Apply it when writing new docs or comments.
