# CANON

A light-blue, blocky/pixelated, menu-driven UI for the **M5Stack Core (Basic)**,
with a sage-mode pixel-art toad companion named **Kawazu Kumite**, a pixelated
Rasengan animation, Bleach-themed Getsuga Tensho / Hollow Mask screens, and a
WiFi captive portal for testing your own devices/network.

## Features

- Bruce-firmware-style UI: every screen transition slides in from the
  right, with in-place updates (selection/scroll/rescans) redrawing instantly
- Pixelated, scrollable menu (10 sections), light-blue themed
- File browser backed by the device's internal flash (SPIFFS) - **no SD card required**
- Push files from your PC to the device over USB serial (no re-flashing needed)
- Kawazu Kumite: an animated pixel-art sage-mode toad (idle bob, blinking, tongue flick)
- Rasengan: an animated pixelated swirling chakra sphere
- Getsuga Tensho: an animated pixelated black crescent of reiatsu with glowing edge and orbiting particles
- Hollow Mask: a pixel-art Hollow mask with pulsing red markings
- WiFi Portal: opens a WiFi access point with a light-blue themed login page, for
  testing how your own devices respond to a captive portal
- WiFi Scanner: lists nearby WiFi networks with signal strength and security status
- BLE Scanner: lists nearby Bluetooth Low Energy devices with name, address, and RSSI
- IR Remote: sends common NEC-protocol TV power codes via the IR LED on GPIO 12
- I2C Scanner: scans the Grove port (pins 21/22) for connected I2C devices
- Boot splash: "TAKE THIS RASENGAN!" with an animated Rasengan on startup
- Battery indicator: live battery percentage (and charging status) in the header bar
- Button click feedback: short tones on every button press

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

**WiFi Portal screen**:
- Opens a WiFi access point named `CANON-Portal` and shows its SSID, IP
  address, and a running count of submitted logins
- Any device that connects to `CANON-Portal` and opens a browser is shown
  a light-blue "WiFi login" page; submitted network/password values are
  appended to `/portal_log.txt` on the device's flash, viewable from the
  Files screen
- `B` — stop the access point and return to menu

> This is a generic test page intended for checking how your own devices
> react to a captive portal. It does not mimic any real service.

**WiFi Scanner screen**:
- Lists nearby networks (SSID, security `*` flag, and RSSI in dBm)
- `A` / `C` — scroll, `B` — back to menu (re-scans each time you open it)

**BLE Scanner screen**:
- Lists nearby BLE devices (name, MAC address, RSSI), 3-second scan
- `A` / `C` — scroll, `B` — back to menu

**IR Remote screen**:
- Select a TV power code and press `B` to transmit it via the IR LED on GPIO 12
- `A` / `C` — move selection, `B` on "<- Back to Menu" returns to the menu
- Codes are commonly published NEC power-toggle codes (Samsung, LG, Vizio,
  generic) - coverage depends on your TV's remote protocol

**I2C Scanner screen**:
- Scans the Grove port (SDA=21, SCL=22) for connected I2C devices and lists
  their addresses
- `A` — back to menu, `B` — rescan

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
