# CANON

A light-blue, blocky/pixelated, menu-driven UI for the **M5Stack Core (Basic)**,
with a sage-mode pixel-art toad companion named **Kawazu Kumite**, a pixelated
Rasengan animation, and Bleach-themed Getsuga Tensho / Hollow Mask screens.

## Features

- Pixelated menu (Files / Pet / Rasengan / Getsuga Tensho / Hollow Mask), light-blue themed
- File browser backed by the device's internal flash (SPIFFS) - **no SD card required**
- Push files from your PC to the device over USB serial (no re-flashing needed)
- Kawazu Kumite: an animated pixel-art sage-mode toad (idle bob, blinking, tongue flick)
- Rasengan: an animated pixelated swirling chakra sphere
- Getsuga Tensho: an animated pixelated black crescent of reiatsu with glowing edge and orbiting particles
- Hollow Mask: a pixel-art Hollow mask with pulsing red markings

## Hardware

- M5Stack Core (Basic), ESP32 + 320x240 ILI9341 TFT
- No SD card needed - files are stored in onboard flash (SPIFFS)

## Building & flashing the firmware

Requires [PlatformIO](https://platformio.org/).

```sh
cd firmware
pio run -t upload
pio device monitor
```

The `m5stack/M5Stack` library is pulled in automatically via `platformio.ini`.

## Controls

**Menu** (default screen):
- `A` — move selection up
- `C` — move selection down
- `B` — open the highlighted item (Files / Pet / Rasengan)

**Files screen**:
- `A` / `C` — scroll the file list
- `B` — back to menu

**Pet, Rasengan, Getsuga Tensho, and Hollow Mask screens**:
- `B` — back to menu

## Uploading files from your PC

With the M5Stack plugged in over USB:

```sh
cd uploader
pip install -r requirements.txt
python upload.py <PORT> file1.txt file2.jpg ...
```

- On Windows, `<PORT>` looks like `COM5`.
- On macOS/Linux, it looks like `/dev/tty.usbserial-XXXX` or `/dev/ttyUSB0`.

Files are written to the device's internal flash (SPIFFS) and the file
browser refreshes automatically after each upload. SPIFFS storage is limited
(a few hundred KB to a couple MB depending on partition layout), so this is
best for small files.

## Theme

Colors are defined in `firmware/include/theme.h`:

- `THEME_BG` — light blue background (`#ADD8E6`)
- `THEME_ACCENT` — header/footer bars and borders
- `THEME_PANEL` / `THEME_SELECT` — list rows and selection highlight
- `FROG_*` — sage-mode orange palette for Kawazu Kumite
- `RASEN_*` — blue palette for the Rasengan animation
- `GETSUGA_*` — dark/cyan palette for the Getsuga Tensho animation
- `HOLLOW_*` — white/red/black palette for the Hollow Mask
