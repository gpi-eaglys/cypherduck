# MCU boot sequence

The steps between power-on and the first line of application code. The generic sequence
applies to any MCU. The second half maps it to the ESP32-S3. See [esp32-s3.md](esp32-s3.md).

## Generic sequence

**1. Power rises.**
The supply voltage rises from 0 V to its nominal value. This takes a few milliseconds. The
chip is not stable during this time.

**2. Reset is asserted.**
A circuit inside the chip holds the CPU in reset. It waits for the supply voltage to reach a
threshold and for the clock to stabilise. This is power-on reset.

**3. Reset is released.**
The CPU begins execution. All registers hold their default values. No RAM contents are valid.

**4. The CPU fetches the reset vector.**
The reset vector is at a fixed address defined by the hardware. The CPU always starts there.
The address cannot be changed by software.

**5. The ROM bootloader executes.**
ROM is written during manufacture and is read-only. The code at the reset vector is the ROM
bootloader. It performs minimal clock and memory setup.

**6. The ROM bootloader reads the strapping pins.**
Strapping pins are sampled once, shortly after reset is released. Their levels select the boot
mode. One mode executes the program in flash. Another mode waits for a new image over USB or
UART. The second mode is how new firmware is written to the device.

**7. The ROM bootloader loads the second-stage bootloader.**
It copies the second-stage bootloader from flash into RAM and jumps to it. The ROM bootloader
does not run again until the next reset.

**8. The second-stage bootloader prepares the chip.**
It raises the clock to the configured frequency and initialises external memory. With secure
boot enabled, it verifies the signature of the application image. Verification failure stops
the boot. It then selects an application partition, loads it, and jumps to its entry point.

**9. The startup code initialises the C runtime.**
The C environment does not exist yet. The startup code performs three operations:

- Sets the stack pointer.
- Copies initialised variables from flash to RAM (the `.data` section).
- Writes zero to uninitialised variables (the `.bss` section).

C code executes correctly only after these operations complete.

**10. The application entry point is called.**
The application configures its peripherals and enters its main loop.

## ESP32-S3

### Stages

| Stage | Location |
|---|---|
| Reset vector | Fixed in hardware |
| ROM bootloader | 384 KB mask ROM |
| Second-stage bootloader | Flash offset `0x0` |
| Partition table | Flash offset `0x8000` |
| Application | Flash offset `0x10000` |
| Application entry point | `app_main()` |

The offsets are the ESP-IDF defaults. The partition table defines the application offset.

### Boot mode selection

The ESP32-S3 has four strapping pins: GPIO0, GPIO3, GPIO45 and GPIO46. Two of them select the
boot mode. The ROM bootloader reads the result from `GPIO_STRAP_REG`.

| Boot mode | GPIO0 | GPIO46 |
|---|---|---|
| SPI boot (default) | 1 | Any |
| Joint download boot | 0 | 0 |

SPI boot executes the image in flash. Joint download boot accepts a new image over
USB-Serial-JTAG, USB OTG or UART. On the ESP32-S3-DevKitC-1 the BOOT button drives GPIO0 low.

The other two strapping pins have separate functions. GPIO45 selects the VDD_SPI voltage.
GPIO3 selects the JTAG signal source.

### SPI boot

SPI boot is the mode that executes the firmware. The name states where the firmware comes
from: an external flash chip on the SPI bus.

The ESP32-S3 die holds 384 KB of ROM and 512 KB of SRAM. It holds no program flash. The
firmware is stored on a separate flash chip. On an ESP32-S3-WROOM-1 module that chip is a
second die inside the same package. The two dies are connected by the SPI bus.

The ROM bootloader reads the flash chip in single-line SPI mode at a low clock frequency.
This mode is supported by every compatible flash chip. The second-stage bootloader then
reconfigures the bus to dual, quad or octal mode, according to the configured flash settings.

The application is not copied to SRAM in full. The cache maps flash into the CPU address
space in 64 KB blocks, and the CPU fetches instructions from those addresses. The cache reads
the flash over SPI on demand. This is execute in place (XIP). For the mapping limits, see
[esp32-s3.md](esp32-s3.md).

Two consequences follow:

- A cache miss costs an SPI read. Code in flash executes slower than code in SRAM, and its
  execution time varies.
- ESP-IDF places a function in SRAM when it is marked `IRAM_ATTR`. Interrupt handlers with
  timing requirements are placed this way.

The other boot mode, joint download boot, receives an image from a host PC over USB or UART.
`esptool.py` and `idf.py flash` use it to write the flash chip, then reset the board into SPI
boot. On the ESP32-S3-DevKitC-1, holding BOOT while pressing RESET selects download boot.

An STM32U5 stores its firmware in flash on the same die as the CPU, at address `0x0800_0000`.
No SPI bus and no cache mapping are involved. This difference applies at M5, where the target
changes.

### Startup functions

ESP-IDF starts FreeRTOS before the application entry point. The sequence after the
second-stage bootloader jumps to the application:

| Function | Action |
|---|---|
| `call_start_cpu0` | Initialises the C runtime, CPU exceptions, memory, clocks. Starts core 1. |
| `start_cpu0` | Initialises the heap, libc, console and registered components. |
| `main_task` | Runs as a FreeRTOS task and calls `app_main`. |

Two consequences follow:

- `app_main` executes inside a FreeRTOS task. It is not bare-metal code.
- `app_main` may return. The main task then terminates and the remaining tasks continue.

Core 0 performs the boot. Core 1 is held in reset until `call_start_cpu0` releases it.

## Relevance to Cypherduck

The host expects a USB device to respond within a defined interval after attachment. Steps 1
to 10 complete before the firmware can answer any request. Boot time is the first measurement
to take if enumeration at M0 fails intermittently.

Steps 5 and 6 also state the limit of the ESP32-C3. Its USB descriptors are held in ROM and
cannot be replaced by the application.

## Sources

- [ESP32-S3 Series Datasheet](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf), section 3.1
- [ESP-IDF Application Startup Flow](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/startup.html)
