# eFuse keys

Key storage on the ESP32-S3. eFuse holds the device key used by the handshake, and the two
values that protect the firmware image. See [handshake.md](handshake.md) and
[esp32-s3.md](esp32-s3.md).

## What eFuse is

One-time-programmable memory on the SoC die. Each bit is a fuse. Burning a bit changes it
from 0 to 1. No operation changes it back.

eFuse is not flash. Flash is a separate chip on the SPI bus. Writing the flash chip does not
change eFuse, and a firmware update does not change eFuse.

## Layout

The ESP32-S3 has 4096 bits of eFuse in eleven blocks, BLOCK0 to BLOCK10. BLOCK1 to BLOCK10
hold 256 bits each. BLOCK0 holds configuration bits.

1792 bits are available to the application:

| Block | Name | Contents |
|---|---|---|
| BLOCK3 | `BLOCK_USR_DATA` | 256 bits, free |
| BLOCK4 to BLOCK9 | `BLOCK_KEY0` to `BLOCK_KEY5` | 256 bits each, key storage |

Six key blocks are available. Cypherduck uses three.

## Key purpose

Each key block has a purpose field. The purpose states which hardware block may use that key.
The purpose is burned at the same time as the key and is permanent.

The purpose is an access control. A key burned for one function cannot be used through
another. This prevents a secret key from being read out by a peripheral that returns its
result to software.

Purposes used by Cypherduck:

| Purpose | Effect |
|---|---|
| `HMAC_UP` | The HMAC peripheral returns its result to firmware |
| `SECURE_BOOT_DIGEST0` | The block holds a secure boot public key digest |
| `XTS_AES_128_KEY` | The block holds the flash encryption key |

Two further purposes exist for the HMAC peripheral. `HMAC_DOWN_DIGITAL_SIGNATURE` routes the
result to the digital signature peripheral. `HMAC_DOWN_JTAG` routes it to the JTAG controller.
In both cases firmware never reads the result. Cypherduck uses neither.

## Read protection and write protection

Two separate bits per block.

Read protection stops software from reading the block. The hardware peripheral assigned by
the purpose field still uses it. A read from software returns zeros.

Write protection stops any further bits in the block from being burned. Existing bits are
unaffected, because burning is one-way in any case.

Both are set during provisioning, before the device leaves the controlled environment.

## The three keys

| Block | Contents | Purpose | Read-protected | Secret |
|---|---|---|---|---|
| One key block | Device key | `HMAC_UP` | Yes | Yes |
| One key block | Digest of the firmware signing public key | `SECURE_BOOT_DIGEST0` | No | No |
| One key block | Flash encryption key | `XTS_AES_128_KEY` | Yes | Yes |

### Device key

256 random bits, unique per device. The source holds a copy, indexed by device ID.

The HMAC peripheral computes `HMAC-SHA256(device key, message)` and returns 32 bytes to
firmware. The key itself is never returned. The handshake uses this operation to derive the
session key. See [handshake.md](handshake.md).

A unique key per device is a requirement. A shared key would mean that extraction from one
unit compromises every unit, and revocation of a lost unit would be impossible.

### Secure boot public key digest

SHA-256 of the RSA-3072 public key that verifies the firmware signature. 32 bytes.

This value is not secret. It is a hash of a public key. It is burned into eFuse so that it
cannot be replaced.

The public key itself is stored in flash, in a signature block appended to the firmware image.
At boot the chip hashes that public key and compares the result against the digest in eFuse.
A mismatch stops the boot. On a match, the chip uses the public key to verify the firmware
signature. See [boot.md](boot.md).

Secure Boot V2 accepts up to three digests, in three blocks, each with its own revocation bit.
Any one of them verifies an image. The digests that may be needed are burned during
provisioning. A device with one digest burned and a lost private key cannot be updated.

### Flash encryption key

The flash controller encrypts and decrypts the external flash chip with XTS-AES. The key is
read-protected and is used only by that controller.

The device generates this key itself on first boot and burns it. The key then exists on no
other machine. An over-the-air update is written by the running firmware as plaintext, and the
flash controller encrypts it during the write, so the key is not required outside the chip.

Generating the key on a provisioning machine is the alternative. It is required only when an
already encrypted image is written over the serial interface.

## What the three keys protect

| Threat | Countermeasure |
|---|---|
| Untrusted PC reads the audio | Device key, through the session key |
| Unsigned firmware executes on the device | Secure boot digest |
| Flash chip is removed and read | Flash encryption key |

Secure boot controls what the chip executes. It does not control what can be read. Flash
encryption controls what can be read. It does not control what executes. Both are enabled.

## Limits

Firmware running on the device reads plaintext flash. Flash decryption is transparent to
software. A code execution fault exposes the image contents.

A physical attacker with laboratory equipment reads eFuse by decapsulation and probing. The
ESP32-S3 has no countermeasure against invasive attacks. This is one of the reasons the
production MCU recommendation is STM32U5, and a secure element removes it entirely. See
[milestones.md](milestones.md).

Burning is irreversible. A wrong value in a key block cannot be corrected, and the chip is
then unusable for its intended purpose.

## Sources

- [eFuse Manager - ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/efuse.html)
- [HMAC - ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/hmac.html)
- [Secure Boot V2 - ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/secure-boot-v2.html)
- [Flash Encryption - ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/flash-encryption.html)
