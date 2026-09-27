# Key exchange

How the source and the device obtain the same session key without transmitting it. The key
is used by the audio path described in [datapath.md](datapath.md).

## The problem

AEAD is symmetric. The source and the device need the same 32 bytes.

The key cannot be transmitted. The PC relay forwards every byte between the two endpoints
and is not trusted with the audio.

The key cannot be a fixed value shared by all units. Extraction from one unit would then
compromise every unit.

## Diffie-Hellman

Both endpoints compute the same value from different inputs. Neither value is transmitted.

The example below uses small numbers. Real parameters are given in the last section.

Two constants are public. An attacker knows both:

```
p = 23      prime modulus
g = 5       base
```

Each endpoint selects a private number and computes a public number from it:

| | Source | Device |
|---|---|---|
| Private value | a = 6 | b = 15 |
| Public value, g^private mod p | 5^6 mod 23 = 8 | 5^15 mod 23 = 19 |

The endpoints exchange the public values. The source transmits 8. The device transmits 19.

Each endpoint raises the received value to its own private value:

| Endpoint | Calculation | Result |
|---|---|---|
| Source | 19^6 mod 23 | 2 |
| Device | 8^15 mod 23 | 2 |

Both results are 2. That value is the shared secret.

## Why the results are equal

Written without the modulus:

```
Source:  (5^15)^6  =  5^(15 x 6)  =  5^90
Device:  (5^6)^15  =  5^(6 x 15)  =  5^90
```

Exponents multiply, and multiplication is commutative. The two endpoints perform different
operations and produce the same value.

## Why the exchange cannot be reversed

The attacker holds p, g, and the two public values 8 and 19. Computing the shared secret
requires a or b.

Recovering a from `5^a mod 23 = 8` is the discrete logarithm problem. With these values the
answer is found by testing every exponent.

The modulus is what makes the problem hard at full scale. The powers of 5 mod 23 are:

```
exponent:  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
result:    5   2  10   4  20   8  17  16  11   9  22  18  21  13  19
```

The results do not increase with the exponent and follow no usable order. The value of the
result states nothing about the size of the exponent.

Without the modulus, `5^6` is 15625. The magnitude of the result would give the exponent
directly.

At the sizes used in practice, reversal requires approximately 2^128 operations. No
method is known that reduces this.

## X25519

X25519 has the same structure. The operation is point multiplication on Curve25519 instead
of exponentiation modulo a prime.

```
G       fixed base point, public
a       private key, 32 random bytes
A = aG  public key
```

The exchange:

```
Source:  a(bG)  =  (ab)G
Device:  b(aG)  =  (ab)G
```

Point multiplication is commutative, so both endpoints compute the same point. Recovering a
from aG is the elliptic curve discrete logarithm problem.

Curve operations require smaller keys than exponentiation modulo a prime for the same
strength:

| Method | Public key size |
|---|---|
| X25519 | 32 bytes |
| Exponentiation mod p | approximately 384 bytes |

The M1 wire rate is 34.4 kB/s. The smaller key reduces handshake traffic on that budget.

## Derivation

The X25519 output is not used as the AEAD key. HKDF derives the session keys from it.

The raw output has mathematical structure that a key derivation function removes. HKDF also
produces more than one key from one exchange, which the audio path requires. See
[datapath.md](datapath.md).

## Authentication

X25519 establishes a shared secret with an unidentified party. It does not state who holds
the other private key.

An intermediate party can run the exchange twice, once with each endpoint, and read all
traffic. Both endpoints observe a successful exchange.

The ephemeral public key is therefore signed with the device private key. The source
verifies the signature against the device certificate, then checks the device ID against
the authorization list. These are the M5 exit criteria. See [milestones.md](milestones.md).

## Cost on the ESP32-S3

The ESP32-S3 has no ECC accelerator. X25519 runs in software.

The exchange runs once per session. It is not part of the per-frame cost that M6 measures.
