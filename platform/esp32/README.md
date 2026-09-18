# ESP32-S3 Platform

Independent ESP-IDF 6.1 target for the verified ESP32-S3 + ST7789 hardware. The
`pet_shared` component compiles Core, App, Animation, Event, Renderer, and generic HAL
sources directly from the repository; no platform-specific copies exist.

## Build

```sh
source /home/lab_726/.espressif/tools/activate_idf_v6.1.sh
cd platform/esp32
idf.py build
```

## Flash And Monitor

```sh
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 monitor
```

Do not use `erase-flash` for normal development and do not modify eFuse. Adjust the
serial port when needed.

## V0.4 Runtime

The firmware initializes the verified ST7789 path with the backlight off, then runs the
shared App, Animation, Event, and Renderer modules continuously. A compiled RGB565 asset
provider supplies boot, idle, blink, look, happy, and sleep resources without a
filesystem or per-frame allocation. The backlight turns on after the first frame.

Core's timer events provide the normal Blink/Look idle sequence. The platform runtime
only adds Happy at 10 seconds, Sleep at 14 seconds, and Wake at 17 seconds in each
22-second demo cycle. These are posted through `pet_app_post_event()`.

The runtime targets a 100 ms frame period using `esp_timer_get_time()`. When synchronous
rendering exceeds the budget, it yields for one RTOS tick instead of busy-waiting.

## V0.5 LCD Performance Work

Every candidate config below ran the full animation demo on target for at least 60
seconds with five-second summaries. Stability is judged from the serial log: no SPI
driver error, no watchdog, no reboot, no queue failure, and constant free heap. Visual
glitch and tearing inspection is still pending human verification.

```text
| Config        | SPI          | staging/block  | DMA | flush avg | FPS  | stable(log) |
|---------------|--------------|----------------|-----|-----------|------|-------------|
| baseline      | 10 MHz       | 64 B           | no  | 149.63 ms | 5.2  | yes         |
| staging 512 B | 10 MHz       | 512 B          | no  | rejected  | -    | -           |
| clock 20 MHz  | 20 MHz       | 64 B           | no  | 103.28 ms | 6.6  | yes         |
| clock 40 MHz  | 40 MHz       | 64 B           | no  | 79.87 ms  | 8.3  | yes         |
| clock 60 MHz  | req.60->40   | 64 B           | no  | 79.87 ms  | 8.3  | yes         |
| clock 80 MHz  | 80 MHz       | 64 B           | no  | 68.60 ms  | 9.0  | yes         |
| DMA 64 B      | 40 MHz       | 64 B           | yes | 85.34 ms  | 7.6  | yes         |
| DMA 512 B     | 40 MHz       | 512 B          | yes | 37.48 ms  | 10.0 | yes         |
| DMA 1 KiB     | 40 MHz       | 1024 B         | yes | 34.04 ms  | 10.0 | yes         |
| DMA 4 KiB     | 40 MHz       | 4096 B         | yes | 31.48 ms  | 10.0 | yes         |
| DMA 8 KiB     | 40 MHz       | 8192 B         | yes | 31.07 ms  | 10.0 | yes         |
| block 1 line  | 40 MHz       | 240x1 lines    | yes | 37.91 ms  | 10.0 | yes         |
| block 4 lines | 40 MHz       | 240x4 lines    | yes | 32.43 ms  | 10.0 | yes         |
| block 8 lines | 40 MHz       | 240x8 lines    | yes | 31.51 ms  | 10.0 | yes         |
| block 16 lines| 40 MHz       | 240x16 lines   | yes | 31.07 ms  | 10.0 | yes         |
| block 32 lines| 40 MHz       | 240x32 lines   | yes | 30.85 ms  | 10.0 | yes         |
```

Notes:

- ESP-IDF rejects a non-DMA transaction larger than the 64-byte
  `SOC_SPI_MAXIMUM_BUFFER_SIZE` (`txdata transfer > host maximum`), so polling staging
  sizes above 64 B cannot run and the config is now rejected at init.
- Requesting 60 MHz landed on the same divider as 40 MHz, confirmed with
  `spi_device_get_actual_freq`; both logged the identical 79.87 ms flush.
- 80 MHz polled stably on the log but only adds ~14% over 40 MHz and has no visual
  confirmation yet, so it is not the selected clock.
- Returns diminish after 4 KiB: 8 KiB saves only 0.42 ms for 4 KiB more internal RAM.
  Line-aligned blocks never beat plain 4 KiB staging meaningfully; the best block result
  needs 15 KiB internal RAM for a 2% flush gain.

## Selected Configuration (V0.5)

| Parameter | Value |
|---|---|
| SPI clock | 40 MHz (actual, verified via `spi_device_get_actual_freq`) |
| DMA | `SPI_DMA_CH_AUTO`, synchronous wait, DMA-capable internal staging |
| Staging buffer | 4096 bytes internal RAM |
| Block mode | disabled (staging-size chunks) |
| Framebuffer | single 115200-byte PSRAM buffer |
| FPS | 10.0 (runtime target bound) |
| flush avg/max | 31.48 / 31.52 ms |
| render avg/max | 37.10 / 37.18 ms |
| frame avg/max | 68.61 / 68.93 ms |
| internal heap | 372315 bytes (stable, 60 s) |
| PSRAM heap | 8269568 bytes (stable, 60 s) |

This is the firmware default. Benchmarks for other configs can be reproduced at build
time:

```sh
idf.py -D PET_LCD_SPI_FREQUENCY_HZ=40000000 \
       -D PET_LCD_STAGING_BUFFER_SIZE=4096 \
       -D PET_LCD_DMA_ENABLED=1 \
       -D PET_LCD_BLOCK_HEIGHT=0 build flash -p /dev/ttyACM0
```

## Diagnostic Modes

Controlled visual A/B testing on the `lcd-visual-diagnosis` branch. Mode and
transport are independent build-time selections; these switches only alter the ESP32
runtime/display backend and never touch Pet Core, Animation, Event, Renderer, or the
ST7789 init/rotation/RGB parameters.

```sh
idf.py -D PET_LCD_DIAG_MODE=STATIC_ONCE -D PET_LCD_DIAG_TRANSPORT=V04 build flash -p /dev/ttyACM0
idf.py -D PET_LCD_DIAG_MODE=STATIC_ONCE -D PET_LCD_DIAG_TRANSPORT=V05 build flash -p /dev/ttyACM0
idf.py -D PET_LCD_DIAG_MODE=STATIC_ONCE -D PET_LCD_DIAG_TRANSPORT=ROW8 build flash -p /dev/ttyACM0
```

| Mode | Behavior |
|---|---|
| `NONE` | Normal pet animation. Use this with `PET_LCD_DIAG_TRANSPORT` to compare animation over the same transport variants. |
| `STATIC_ONCE` | Draws the fixed test pattern, flushes it exactly once, backlight stays on, then never touches the display again. Logs `render_count=1 flush_count=1` and periodic alive messages to prove no further flush. |
| `STATIC_REPEAT` | Draws the same fixed test pattern once, then re-flushes the unchanged framebuffer every 100 ms at ~10 Hz. Logs `render_count=1` with a steadily increasing `flush_count`. |

| Transport | LCD link |
|---|---|
| `DEFAULT` | Uses `PET_LCD_SPI_FREQUENCY_HZ`, `PET_LCD_STAGING_BUFFER_SIZE`, `PET_LCD_DMA_ENABLED`, and `PET_LCD_BLOCK_HEIGHT`. |
| `V04` | 10 MHz, DMA off, 64-byte polling transactions. |
| `V05` | 40 MHz, DMA on, 4096-byte staging. For the 115200-byte framebuffer this is 29 transactions with a final 512-byte tail. |
| `ROW8` | 40 MHz, DMA on, 3840-byte staging, strict 8 lines/block. The 240-line panel transfers as 30 full 3840-byte blocks with no partial tail. |

Startup prints `LCD DIAGNOSTIC MODE: <mode>`, `LCD DIAGNOSTIC TRANSPORT:
<transport>`, the configured link (`LCD: SPI=... staging=... DMA=... block=...`),
the framebuffer byte count, transaction count, final transaction length, and the
actual SPI frequency from `spi_device_get_actual_freq`. `V05` also prints
`final_tail=512 bytes`; `ROW8` prints `blocks=30 block_bytes=3840 partial_tail=0
bytes`.

The static diagnostic framebuffer is identical for every transport: black background,
four corner color blocks, a central cross, a bright 32-line bottom band, and a
separate color on the last line. GPIO7 backlight is set once after init and never
toggled again.

For a temporary animation pacing check, lower the ESP32 runtime frame period without
changing shared code:

```sh
idf.py -D PET_LCD_DIAG_MODE=NONE -D PET_LCD_DIAG_TRANSPORT=V05 \
       -D PET_ESP32_TARGET_FRAME_US=50000 build flash -p /dev/ttyACM0
```

## Memory And Transfer

- Framebuffer: 240 x 240 x RGB565 = 115200 bytes in PSRAM.
- Renderer pixels: logical RGB565 `RRRRRGGGGGGBBBBB`.
- ST7789 transfer: MSB first, converted only in the ESP32 flush path.
- SPI: SPI2, mode 0, 40 MHz, DMA enabled, synchronous wait per transaction.
- Staging: 4096 bytes DMA-capable internal RAM; PSRAM pixels are packed into it per chunk.

Double buffering, dirty regions, and TE synchronization are deferred until separately
verified.
