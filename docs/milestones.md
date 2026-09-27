# Milestones

Development order for the Cypherduck audio dongle. Each milestone has exit criteria
that can be tested. A milestone is complete when every criterion passes.

Design plan: https://claude.ai/code/artifact/d80f3c91-aba6-4cbf-b79a-017f8b0fec15

## Hardware

| Milestone | Hardware |
|----------|---|
| M0 to M4 | ESP32-S3 dev board (for example ESP32-S3-DevKitC-1), PCM5102A module with 3.5 mm jack |
| M5 to M7 | B-U585I-IOT02A, same PCM5102A module |

One board covers M0 to M4. The ESP32-S3 has both a native USB controller and an I2S
peripheral, so no second prototyping board is needed.

The PCM5102A module requires three settings in addition to power and the three I2S
signals: SCK to GND, XSMT high, FMT low.

Connect the device to the native USB port (GPIO19 and GPIO20). The console runs over the
second port, which reaches the USB-to-UART bridge chip on UART0. The USB-Serial-JTAG
controller shares one transceiver with USB OTG and is unavailable while the native port
is in use. See [esp32-s3.md](esp32-s3.md).

ESP32-C3 is not a substitute. Its USB Serial/JTAG controller is fixed-function hardware
with hardcoded descriptors and cannot present a vendor class, which rules out M0, M1, M3
and M4. Its I2S peripheral would run M2 alone, but splitting one milestone onto a second
board buys nothing.

ESP32-S3 is a prototyping target. The production MCU recommendation is STM32U5. See
section 8 of the design plan. Milestones M0 to M4 use no security hardware, so no work
is lost at the move.

## Order

M2 needs no USB and has no dependency on M0 or M1, so it can be done first or in
parallel. The remaining milestones run in the order listed.

## M0 - Enumeration

The device enumerates as a vendor-class USB device. No audio path. No bulk transfers.

Exit criteria:

- Windows binds WinUSB without an installer and without an INF file.
- Linux and macOS claim interface 0 through libusb.
- `cd-send --probe` prints the firmware version returned by GET_INFO.
- The UART log prints the enumeration sequence.

## M1 - Data path

The host sends frames on endpoint 0x01. The device returns status packets on
endpoint 0x81. The frame payload is discarded.

Exit criteria:

- 34.4 kB/s sustained for 10 minutes.
- Sequence numbers in the status reply are contiguous. No gaps.
- The control transfer round-trip time is measured and recorded on Windows, Linux
  and macOS.

## M2 - Audio output

The device generates a tone and outputs it through I2S to the DAC. USB is not used.

Exit criteria:

- A 1 kHz tone is audible at the jack.
- The measured output frequency is recorded. The absolute value is not critical.
- No audible click at output start and at output stop.

## M3 - End to end, plaintext

The host sends unencrypted PCM frames. The device plays them.

Exit criteria:

- A speech file plays for 10 minutes with no dropout.
- Buffer occupancy stays within 20 ms of the configured target.
- The underrun counter returns zero.

## M4 - Transport control

PLAY, PAUSE and STOP as control requests on endpoint 0.

Exit criteria:

- The pause response time is measured with a scope, comparing the jack output
  against the control transfer. The measured value is 10 ms or less.
- No audible click at pause and at resume.
- The response time is unchanged when target_ms is set to 250.
- played_seq in the status reply identifies the last frame sent to the DAC.
- Playback after STOP starts within 40 ms.

## M5 - Provisioning and handshake

Move to STM32U5. Inject a per-device key and certificate. Run the X25519 handshake
through the PC relay.

Exit criteria:

- The handshake completes and both sides derive the same session key.
- The device rejects audio frames received before the handshake completes.
- The source rejects a device whose device ID is not in the authorization list.
- The debug port is fused on one unit. The private key cannot be read from that unit.
- The provisioning sequence runs as one atomic operation: flash, provision, test, fuse.

## M6 - Encrypted audio path

Enable AEAD decryption and the replay window.

Exit criteria:

- Encrypted speech plays with no dropout.
- The worst-case decryption time per frame is measured and recorded.
- A replayed frame is rejected and the error is reported in the status reply.
- A frame with a modified header fails authentication.
- The source transmits at a constant rate. Silence is padded, not omitted.

## M7 - Soak test

Exit criteria:

- 24 hours of continuous playback. The underrun counter returns zero.
- Buffer occupancy is stable across the full run.
- Suspend and resume succeed.
- Unplug and replug succeed.

## Open decision: when to write the second port

M0 to M4 run on ESP32-S3. M5 moves to STM32U5. The portability boundary described in
section 4 of the design plan is therefore not verified until M5.

Two options:

1. Buy the B-U585I-IOT02A now and complete M0 on both targets. This verifies the port
   boundary while the shared core is small.
2. Write the STM32U5 port at M5. This defers the hardware cost and accepts rework if
   MCU-specific assumptions have entered the shared core.

Option 1 costs one board. Option 2 costs an unknown amount of rework. Select before M1.
