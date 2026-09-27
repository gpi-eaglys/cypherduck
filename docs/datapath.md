# Audio data path

The route of one audio frame, from the source to the analogue output. This is the state at
M6. Earlier milestones implement parts of it. See [milestones.md](milestones.md).

## Participants

| Name | Role |
|---|---|
| Source | Produces the audio. Holds the authorization list. Encrypts each frame. |
| PC relay | Forwards frames over USB. Holds no session key. Cannot read the audio. |
| Device | Decrypts each frame and drives the DAC. |

The source and the device are the two endpoints. The PC relay is transport. A frame is
encrypted before it reaches the relay and is decrypted after it leaves the relay.

## Route

```
Source
  1  PCM samples
  2  AEAD encrypt           -> header + ciphertext + tag
       |
       |  PC relay
       |  USB full-speed, bulk OUT, endpoint 0x01
       v
Device (ESP32-S3)
  3  USB OTG peripheral     -> FIFO
  4  DMA                    -> SRAM
  5  Replay window check    -> duplicate frames rejected
  6  AEAD decrypt and verify-> plaintext PCM in SRAM
  7  Ring buffer            -> absorbs rate difference
  8  GDMA                   -> feeds I2S
  9  I2S peripheral         -> BCK, WS and DATA pins
       |
       |  16-bit, Philips format
       v
PCM5102A
 10  Delta-sigma modulator
 11  Analogue low-pass filter
       |
       v
 12  3.5 mm jack
```

Status packets travel in the other direction, on endpoint 0x81.

## Frame structure

Each frame has three parts:

| Part | Encrypted | Authenticated |
|---|---|---|
| Header: sequence number, length, flags | No | Yes |
| Payload: PCM samples | Yes | Yes |
| Authentication tag | No | Is the authentication |

The header is transmitted in clear text. The device reads the sequence number before it
decrypts anything. The header is passed to the AEAD function as associated data, so a
modified header fails verification.

The tag is 16 bytes with AES-GCM. The design plan defines the field widths.

## Order of operations on receipt

The order is a requirement, not a preference:

1. Read the sequence number from the header.
2. Compare it against the replay window. A duplicate or out-of-window value is rejected
   here. No decryption is performed.
3. Decrypt the payload and verify the tag.
4. On verification failure, discard the frame, increment the error counter, and report the
   error in the next status packet.
5. On success, write the samples to the ring buffer.

Step 5 runs only after step 3 completes. Decryption produces bytes before verification
finishes. Those bytes are not audio until the tag verifies.

Tag comparison uses a constant-time function. A byte-by-byte comparison returns early and
its execution time states how many bytes matched.

## AEAD

AEAD is Authenticated Encryption with Associated Data. It provides two properties in one
operation:

- Confidentiality. The payload cannot be read without the key.
- Authenticity. Neither the payload nor the header can be modified without detection.

Encryption and decryption have this form:

```
encrypt(key, nonce, plaintext, associated_data) -> ciphertext, tag
decrypt(key, nonce, ciphertext, associated_data, tag) -> plaintext | FAILURE
```

Decryption returns the plaintext or it fails. There is no third result and no partial
result.

Encryption without authentication is not sufficient. A stream cipher produces ciphertext
that is the plaintext combined with a keystream. An attacker who cannot read the payload
can still modify bits in it, and the receiver accepts the result. The tag detects this.

### Algorithm

AES-GCM. The ESP32-S3 and the STM32U5 both accelerate AES in hardware, so the per-frame
cost is comparable across the M5 target change. M6 records the worst-case decryption time
per frame.

ChaCha20-Poly1305 is the alternative when no AES accelerator is present. That does not
apply to either target.

### Nonce

A nonce is a number used once. It is not secret. It must be unique for a given key.

Reusing a nonce with AES-GCM allows an attacker to recover the value used to generate tags.
Frames can then be forged. This is the one failure that breaks the whole construction.

The nonce is derived from the frame sequence number. The sequence numbers are contiguous
within a session, which M1 verifies. The nonce is not transmitted, because the receiver
reads the sequence number from the header.

## Keys

Three keys with three different lifetimes.

| Key | Lifetime | Stored in | Purpose |
|---|---|---|---|
| Device private key | Permanent | eFuse or secure element | Identifies the device |
| Ephemeral X25519 key pair | One session | SRAM | Produces the shared secret |
| Session key | One session | SRAM | The AEAD key |

The session key is the key that decrypts audio. It is never transmitted. Both endpoints
compute it from the X25519 exchange and derive it with HKDF. See
[key-exchange.md](key-exchange.md).

Two session keys are derived, one per direction. Both directions number their frames from
the same starting value. Separate keys prevent the same key and nonce pair from being used
twice.

The ephemeral key pair is discarded at the end of the session. An attacker who extracts the
device private key later cannot decrypt a recorded session, because the ephemeral keys no
longer exist. This property is forward secrecy.

X25519 establishes a shared secret but does not identify the other endpoint. The ephemeral
public key is signed with the device private key, and the source verifies that signature
against the device certificate. The source then checks the device ID against the
authorization list. Without these two steps an intermediate party can run the exchange with
each side separately and read all traffic.

## Plaintext scope

Plaintext audio exists in SRAM, between step 6 and step 9.

It is not written to flash. SRAM is volatile, so removing power erases it. No flash write
occurs in the audio path. This also satisfies the timing requirement below.

## Timing

Steps 3 and 4 deliver data at the rate of the USB host. Step 8 consumes data at the rate of
the audio clock. The two rates are not equal.

The I2S peripheral has the fixed deadline. Once started it requires a sample every clock
period. An empty ring buffer at that moment is an underrun, and the output is audible as a
gap. M3 and M7 require the underrun counter to return zero.

The ring buffer absorbs the difference. `target_ms` sets its occupancy target. A larger
value tolerates more jitter and increases the delay between a PAUSE request and silence at
the jack. M4 requires that delay to be 10 ms or less, and requires the same result at
`target_ms` set to 250.

The cache is shared by both CPU cores, and a flash write disables it. Code that runs during
a flash write executes from SRAM and is marked `IRAM_ATTR`. A flash write in the audio path
stalls both cores and causes an underrun. See [esp32-s3.md](esp32-s3.md).

## Limit of protection

After step 11 the signal is an analogue voltage. No cryptography applies to it. Physical
access to the jack or to the wiring is sufficient to record the audio.

Encryption protects the audio between the source and the device. The analogue output is
outside that boundary.

## Related documents

- [milestones.md](milestones.md)
- [esp32-s3.md](esp32-s3.md)
- [boot.md](boot.md)
