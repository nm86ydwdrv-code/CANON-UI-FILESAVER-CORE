# CANON

A light-blue, Flipper-Zero-style file browser for the **M5Stack Core (Basic)**,
with a sage-mode frog companion named **Kawazu Kumite** living on its own screen.

## Features

- File browser UI for an SD card, styled with a light-blue Flipper Zero-inspired theme
- Push files from your PC to the device over USB serial (no re-flashing needed)
- A small animated frog pet (idle bob, blinking, tongue flick, sage-mode markings)

## Hardware

- M5Stack Core (Basic), ESP32 + 320x240 ILI9341 TFT
- A FAT32-formatted microSD card inserted in the Core's SD slot

## Building & flashing the firmware

Requires [PlatformIO](https://platformio.org/).

```sh
cd firmware
pio run -t upload
pio device monitor
```

The `m5stack/M5Stack` library is pulled in automatically via `platformio.ini`.

## Controls

**File browser** (default screen):
- `A` — move selection up
- `C` — move selection down
- `B` — switch to the pet screen

**Pet screen**:
- `B` — switch back to the file browser

## Uploading files from your PC

With the M5Stack plugged in over USB:

```sh
cd uploader
pip install -r requirements.txt
python upload.py <PORT> file1.txt file2.jpg ...
```

- On Windows, `<PORT>` looks like `COM5`.
- On macOS/Linux, it looks like `/dev/tty.usbserial-XXXX` or `/dev/ttyUSB0`.

Files are written to `/uploads/<filename>` on the SD card and the file
browser refreshes automatically after each upload.

## Theme

Colors are defined in `firmware/include/theme.h`:

- `THEME_BG` — light blue background (`#ADD8E6`)
- `THEME_ACCENT` — header/footer bars
- `THEME_PANEL` / `THEME_SELECT` — list rows and selection highlight
- `FROG_*` — sage-mode green/orange palette for Kawazu Kumite
