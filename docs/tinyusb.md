# TinyUSB

Why the project uses a third-party USB device stack, what part of it is in the
repository, and how to upgrade it. See [milestones.md](milestones.md) and
[esp32-s3.md](esp32-s3.md).

## Why a USB stack is used

A USB device must implement the protocol before it can carry any data. That work is the
same for every product:

- The device state machine: default, addressed, configured.
- The standard requests defined by chapter 9 of the USB specification: `SET_ADDRESS`,
  `GET_DESCRIPTOR`, `SET_CONFIGURATION`, `GET_STATUS`, `CLEAR_FEATURE`.
- Control transfers: setup stage, data stage, status stage, short packets and zero-length
  packets.
- Endpoint and FIFO management, which differs per MCU.
- A device controller driver for each chip.

TinyUSB implements all of it. Cypherduck supplies the descriptors, the class 0xFF
endpoint handling and the application logic.

## Why one stack instead of two

Espressif and ST each supply a USB device library for their own parts. Using both would
produce two implementations of the same device behind two different APIs, and `core/`
would no longer be shared between the targets.

TinyUSB presents one API on both chips. This is the portability boundary defined in
section 4 of the design plan: code above it contains no MCU-specific conditionals, and a
target is selected by linking a different port library.

ESP-IDF's own USB device support is TinyUSB, repackaged as a component. Using it would
not avoid the library. It would only change where the copy comes from.

## What is in the repository

```
third_party/tinyusb/        3.8 MB, 212 files
third_party/tinyusb.VERSION upstream URL, release tag, commit hash, deletions
```

Only `src/` is kept. Release 0.18.0, commit `86ad6e5`.

The copy is plain files, not a submodule. Two reasons: a clone needs no extra step, and a
build of an old firmware version does not depend on the upstream repository still being
reachable and the tag still existing.

Parts of `src/` used by this project:

| Path | Function |
|---|---|
| `src/device/` | Device state machine and control transfers |
| `src/class/vendor/` | Class 0xFF driver, bulk endpoints |
| `src/class/audio/` | Audio class driver, if the plaintext function is added |
| `src/portable/synopsys/dwc2/` | Device controller driver. Both targets use a DWC2 core. |
| `src/osal/` | Operating system abstraction. FreeRTOS on ESP-IDF. |

## Upgrading

1. Clone the new release into a temporary directory.
2. Record the tag and commit hash.
3. Delete everything except `src/`, `LICENSE` and the top-level description files.
4. Replace `third_party/tinyusb/` and update `third_party/tinyusb.VERSION`.

`third_party/tinyusb.VERSION` lists the deletions so that step 3 is repeatable.

## Sources

- [TinyUSB](https://github.com/hathach/tinyusb)
