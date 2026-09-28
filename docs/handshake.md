# Handshake with two-way nonces

How the source and the device derive the same session key from a per-device key held in
eFuse. The session key is used by the audio path described in [datapath.md](datapath.md).

This is the symmetric handshake. It applies to the ESP32-S3 target. The X25519 handshake in
[key-exchange.md](key-exchange.md) applies to the STM32U5 target. The last section states the
difference.

## Preconditions

| Party | Holds |
|---|---|
| Source | The device key of every authorized device, indexed by device ID |
| Device | Its own device key, in eFuse, read-protected |
| PC relay | Nothing |

The device key is 256 bits and is unique per device. It is burned during provisioning. See
[efuse.md](efuse.md).

The device key is never transmitted. Firmware cannot read it. The HMAC peripheral uses it and
returns only the result.

## Sequence

```
Source                          PC relay                        Device

                                                          1  generate N_d
   3  generate N_s      <----  device ID, N_d       <----  2  send
   4  send              ---->  N_s                  ---->
   5  derive K_session                                    5  derive K_session
```

1. The device generates `N_d`, a fresh random value.
2. The device sends its device ID and `N_d`. Both are in clear text.
3. The source looks up the device key for that device ID. It generates `N_s`, a fresh random
   value.
4. The source sends `N_s`. It is in clear text.
5. Both parties compute `K_session = HMAC-SHA256(device key, N_s || N_d)`.

`||` is concatenation. The order is fixed. Both parties use `N_s` followed by `N_d`.

The HMAC output is 32 bytes. AES-256 uses all of them.

## Why both nonces are required

The relay forwards every byte and can record and repeat any of them.

`N_s` alone is not sufficient. The relay records a session, then sends the same `N_s` again in
a later session. The device derives the same session key and accepts the recorded audio a
second time.

`N_d` prevents this. The device generates it fresh in every session, and the relay cannot
influence it. A repeated `N_s` produces a different session key, the authentication tag fails,
and the recorded frames are discarded.

## What the relay observes

| Value | Visible to the relay | Useful to the relay |
|---|---|---|
| Device ID | Yes | No. It is an identifier, not a secret. |
| `N_d` | Yes | No |
| `N_s` | Yes | No |
| Device key | No | - |
| `K_session` | No | - |

Both nonces are transmitted in clear text. They are inputs to an HMAC whose key the relay does
not hold. Recovering `K_session` from them requires the device key.

## Authentication

The handshake authenticates both parties, with no additional step.

Only a party holding the device key can derive `K_session`. The source verifies the
authentication tag on every status packet from the device. The device verifies the tag on
every audio frame from the source. A party without the device key produces frames that fail
verification.

An intermediate party cannot substitute its own audio stream, because it cannot produce a
valid tag.

## Session key use

One HMAC output produces one 32-byte value. The audio path requires two keys, one per
direction. See [datapath.md](datapath.md).

Two options produce them:

1. Two HMAC calls with different constants appended, for example `N_s || N_d || 0x00` and
   `N_s || N_d || 0x01`.
2. One HMAC call, followed by HKDF expansion into two keys.

Option 1 uses the hardware peripheral for both keys and requires no software hash. Option 2
matches the derivation used by the X25519 handshake.

## Revocation

The source holds one key per device ID. Revoking a lost device is the removal of its entry.
The device then receives no further `N_s`, and no session starts.

Revocation applies to future sessions. A recorded session remains decryptable by anyone who
later extracts the device key from that unit.

## Limits

**No forward secrecy.** The session key is a function of the device key and two public values.
An attacker who records sessions and later extracts the device key decrypts every recorded
session.

**The source holds every device key.** A compromise of the source exposes all devices. The
source is inside the trusted boundary, so this is consistent with the threat model, but it
concentrates the consequence of a breach.

**The device key is in eFuse without invasive attack countermeasures.** See
[efuse.md](efuse.md).

## Difference from the X25519 handshake

| | Symmetric handshake | X25519 handshake |
|---|---|---|
| Secret held by the device | Device key, 256 bits | Device private key |
| Secret held by the source | A copy of every device key | Public certificates only |
| Forward secrecy | No | Yes |
| Hardware support on ESP32-S3 | HMAC peripheral | None. Software only. |
| Compromise of the source | Exposes all devices | Exposes no device key |

The ESP32-S3 has no ECC accelerator and no hardware storage for an ECC private key. A private
key for X25519 is held in flash and is readable by firmware. The symmetric handshake keeps the
secret inside a hardware block on this chip.

The STM32U5 has a public key accelerator, and a secure element stores an ECC private key
without exporting it. The X25519 handshake applies there and provides forward secrecy, which
the symmetric handshake cannot.

## Related documents

- [efuse.md](efuse.md)
- [key-exchange.md](key-exchange.md)
- [datapath.md](datapath.md)
- [milestones.md](milestones.md)
