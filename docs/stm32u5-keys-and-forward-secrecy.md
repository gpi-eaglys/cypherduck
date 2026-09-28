# STM32U5 key storage and forward secrecy

Two answers kept for reference. See [handshake.md](handshake.md) and
[key-exchange.md](key-exchange.md).

## Symmetric scheme on STM32U5

The symmetric handshake can be implemented on the STM32U5, but the mechanism differs. The
STM32U5 has no eFuse.

Its equivalent is the SAES peripheral, which can use two keys that software can use but not
read:

| Key | Property |
|---|---|
| DHUK | Derived from the hardware unique key. Device-unique, non-volatile. |
| BHK | Boot hardware key. Volatile, tamper-protected, loaded at boot. |

One difference matters. The STM32U5 HASH peripheral computes HMAC, but it takes its key from
software. There is no HMAC-with-hardware-key equivalent to the ESP32 block. So the derivation
uses an AES-based function inside SAES instead of HMAC-SHA256, or the device key is stored
wrapped by DHUK and unwrapped inside the peripheral.

The scheme is implementable. It is not needed there, because the STM32U5 has the accelerator
and the key storage for X25519, which gives forward secrecy.

## Forward secrecy

A property: compromise of a long-term key does not expose past sessions.

**Without it**, as in the symmetric scheme: `K_session = HMAC(K_dev, N_s || N_d)`. Both nonces
travel in clear text. An attacker who records a year of ciphertext, then later extracts
`K_dev` from a lost dongle, recomputes every past session key and decrypts all of it.

**With it**, as in X25519: each session uses a temporary key pair, generated at the start and
destroyed at the end. The session key depends on those temporary keys, not only on the
long-term key. Once they are destroyed, the session key cannot be reconstructed. The same
attacker, with the same recordings and the same extracted long-term key, decrypts nothing.

The long-term key still identifies the device. It no longer unlocks the past.

## Sources

- [STM32U5 Symmetric crypto (SAES)](https://www.st.com/content/ccc/resource/training/technical/product_training/group1/c5/ac/f4/97/13/2e/47/78/STM32U5-Security-Symmetric-crypto_SYMCRYPTO/files/STM32U5-Security-Symmetric-crypto_SYMCRYPTO.pdf/_jcr_content/translations/en.STM32U5-Security-Symmetric-crypto_SYMCRYPTO.pdf)
- [Sensitive key protection - ST wiki](https://wiki.st.com/stm32mcu/wiki/Security:Sensitive_key_protection)
