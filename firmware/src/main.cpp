#include <M5Stack.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <Wire.h>
#include <NimBLEDevice.h>
#include "theme.h"
#include "frog.h"
#include "rasengan.h"
#include "bleach.h"
#include "captive_portal.h"

// ---------------------------------------------------------------------
// CANON - M5Stack Core firmware
//   - Light-blue, blocky/pixelated menu-driven UI
//   - Files live on the device's internal flash (SPIFFS) - no SD card
//   - Files are pushed from a PC over USB serial (see uploader/upload.py)
//   - Kawazu Kumite (sage toad pet), pixel Rasengan, and Bleach-themed
//     Getsuga Tensho / Hollow Mask screens
// ---------------------------------------------------------------------

#define MAGIC_LEN 4
const char MAGIC[MAGIC_LEN] = {'K', 'K', 'U', '1'};

enum AppState {
    STATE_MENU,
    STATE_FILES,
    STATE_PET,
    STATE_RASENGAN,
    STATE_GETSUGA,
    STATE_HOLLOW,
    STATE_PORTAL,
    STATE_WIFI_SCAN,
    STATE_BLE_SCAN,
    STATE_IR,
    STATE_I2C
};
AppState state = STATE_MENU;

Frog frog;
Rasengan rasengan;
GetsugaTensho getsuga;
HollowMask hollowMask;
CaptivePortal portal;
const char* PORTAL_AP_NAME = "CANON-Portal";
int lastPortalCaptures = -1;

// File list state
String fileNames[64];
int fileCount = 0;
int selectedFile = 0;
int scrollOffset = 0;
const int VISIBLE_ROWS = 6;
const int ROW_HEIGHT = 24;
const int LIST_TOP = 30;

// Menu state
const char* MENU_ITEMS[] = {
    "Files",
    "Pet: Kawazu Kumite",
    "Rasengan",
    "Getsuga Tensho",
    "Hollow Mask",
    "WiFi Portal",
    "WiFi Scanner",
    "BLE Scanner",
    "IR Remote",
    "I2C Scanner",
};
const int MENU_COUNT = 10;
const int MENU_ITEM_H = 34;
const int MENU_ITEM_GAP = 37;
const int MENU_ICON_PIXEL = 3;
const int MENU_VISIBLE = 5;
int menuScroll = 0;
int selectedMenu = 0;

// ---------------------------------------------------------------------
// Pixel icons (8x8 grids, '.' = transparent, digits index into a palette)
// ---------------------------------------------------------------------

const char* ICON_FOLDER[8] = {
    "........",
    "..0000..",
    ".0111110",
    ".0111111",
    ".0111111",
    ".0111111",
    ".0000000",
    "........",
};
uint16_t PALETTE_FOLDER[4] = {THEME_ACCENT, THEME_PANEL, 0, 0};

const char* ICON_FROG[8] = {
    "........",
    "..0000..",
    ".0111110",
    ".0232320",
    ".0111110",
    ".0111110",
    ".0000000",
    "........",
};
uint16_t PALETTE_FROG[4] = {FROG_DARK, FROG_BODY, FROG_EYE, FROG_PUPIL};

const char* ICON_RASEN[8] = {
    "........",
    "..0000..",
    ".011110.",
    ".012210.",
    ".012210.",
    ".011110.",
    "..0000..",
    "........",
};
uint16_t PALETTE_RASEN[4] = {RASEN_OUTER, RASEN_MID, RASEN_CORE, 0};

const char* ICON_GETSUGA[8] = {
    "........",
    ".000....",
    "01110...",
    "011110..",
    "0111110.",
    "011110..",
    "01110...",
    ".000....",
};
uint16_t PALETTE_GETSUGA[4] = {GETSUGA_GLOW, GETSUGA_DARK, 0, 0};

const char* ICON_HOLLOW[8] = {
    "........",
    ".0000...",
    "0111110.",
    "0121210.",
    "0111110.",
    "0133310.",
    "0111110.",
    ".0000...",
};
uint16_t PALETTE_HOLLOW[4] = {HOLLOW_DARK, HOLLOW_WHITE, HOLLOW_RED, HOLLOW_DARK};

const char* ICON_WIFI[8] = {
    "........",
    "..0000..",
    ".0....0.",
    "..0000..",
    "...01...",
    "..0...0.",
    "...01...",
    "....1...",
};
uint16_t PALETTE_WIFI[4] = {THEME_ACCENT, THEME_TEXT, 0, 0};

uint16_t PALETTE_WIFI_SCAN[4] = {THEME_TEXT, THEME_ACCENT, 0, 0};

const char* ICON_BLE[8] = {
    "........",
    "...0....",
    "..00....",
    ".0.0.0..",
    "..000...",
    ".0.0.0..",
    "..00....",
    "...0....",
};
uint16_t PALETTE_BLE[4] = {THEME_ACCENT, 0, 0, 0};

const char* ICON_IR[8] = {
    "....1.1.",
    "...1...1",
    "..000...",
    ".00000..",
    ".00000..",
    ".00000..",
    ".00000..",
    "........",
};
uint16_t PALETTE_IR[4] = {THEME_ACCENT, THEME_TEXT, 0, 0};

const char* ICON_I2C[8] = {
    ".1.11.1.",
    ".000000.",
    "10000001",
    "10000001",
    "10000001",
    "10000001",
    ".000000.",
    ".1.11.1.",
};
uint16_t PALETTE_I2C[4] = {THEME_ACCENT, THEME_TEXT, 0, 0};

void drawIcon(TFT_eSPI& gfx, int x, int y, const char* rows[8], uint16_t* palette, int pixel = 4) {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char ch = rows[r][c];
            if (ch == '.') continue;
            uint16_t color = palette[ch - '0'];
            gfx.fillRect(x + c * pixel, y + r * pixel, pixel, pixel, color);
        }
    }
}

// ---------------------------------------------------------------------
// UI chrome
// ---------------------------------------------------------------------

void drawHeader(const char* title) {
    M5.Lcd.fillRect(0, 0, 320, 24, THEME_ACCENT);
    M5.Lcd.setTextColor(THEME_SELTEXT, THEME_ACCENT);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(8, 4);
    M5.Lcd.print(title);
}

void drawFooter(const char* a, const char* b, const char* c) {
    M5.Lcd.fillRect(0, 216, 320, 24, THEME_ACCENT);
    M5.Lcd.setTextColor(THEME_SELTEXT, THEME_ACCENT);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(8, 220);
    M5.Lcd.print(a);
    M5.Lcd.setCursor(120, 220);
    M5.Lcd.print(b);
    M5.Lcd.setCursor(232, 220);
    M5.Lcd.print(c);
}

// ---------------------------------------------------------------------
// Bruce-style slide transitions: screen content (between header and
// footer) is drawn into an off-screen canvas, then slid in from the
// right. In-place updates (scrolling, selection) just redraw the canvas
// without sliding.
// ---------------------------------------------------------------------

// The content area (between header and footer) is 192px tall, but a single
// 320x192x16bpp sprite (~123KB) doesn't reliably fit in the largest
// contiguous free heap block on the M5Stack Core (no PSRAM, ~110KB).
// Instead we use one 320x96 sprite and render each screen in two vertical
// passes (top half / bottom half), pushing each half to its place on the
// real display.
TFT_eSprite canvas = TFT_eSprite(&M5.Lcd);
const int CANVAS_TOP = 24;
const int CANVAS_H = 96;            // sprite height (one half of content area)
const int CANVAS_TOP2 = CANVAS_TOP + CANVAS_H; // top of the second half

typedef void (*ContentFn)(int yShift);

void renderHalves(ContentFn fn, int x) {
    fn(0);
    canvas.pushSprite(x, CANVAS_TOP);
    fn(CANVAS_H);
    canvas.pushSprite(x, CANVAS_TOP2);
}

void slideInContent(ContentFn fn) {
    for (int x = 320; x > 0; x -= 40) {
        renderHalves(fn, x);
    }
    renderHalves(fn, 0);
}

void pushContent(ContentFn fn) {
    renderHalves(fn, 0);
}

// ---------------------------------------------------------------------
// Menu screen
// ---------------------------------------------------------------------

void drawMenuIcon(TFT_eSPI& gfx, int i, int x, int y) {
    switch (i) {
        case 0: drawIcon(gfx, x, y, ICON_FOLDER, PALETTE_FOLDER, MENU_ICON_PIXEL); break;
        case 1: drawIcon(gfx, x, y, ICON_FROG, PALETTE_FROG, MENU_ICON_PIXEL); break;
        case 2: drawIcon(gfx, x, y, ICON_RASEN, PALETTE_RASEN, MENU_ICON_PIXEL); break;
        case 3: drawIcon(gfx, x, y, ICON_GETSUGA, PALETTE_GETSUGA, MENU_ICON_PIXEL); break;
        case 4: drawIcon(gfx, x, y, ICON_HOLLOW, PALETTE_HOLLOW, MENU_ICON_PIXEL); break;
        case 5: drawIcon(gfx, x, y, ICON_WIFI, PALETTE_WIFI, MENU_ICON_PIXEL); break;
        case 6: drawIcon(gfx, x, y, ICON_WIFI, PALETTE_WIFI_SCAN, MENU_ICON_PIXEL); break;
        case 7: drawIcon(gfx, x, y, ICON_BLE, PALETTE_BLE, MENU_ICON_PIXEL); break;
        case 8: drawIcon(gfx, x, y, ICON_IR, PALETTE_IR, MENU_ICON_PIXEL); break;
        case 9: drawIcon(gfx, x, y, ICON_I2C, PALETTE_I2C, MENU_ICON_PIXEL); break;
    }
}

void drawMenuContent(int yShift) {
    canvas.fillSprite(THEME_BG);

    for (int row = 0; row < MENU_VISIBLE; row++) {
        int i = menuScroll + row;
        if (i >= MENU_COUNT) break;

        int y = (LIST_TOP - CANVAS_TOP - yShift) + row * MENU_ITEM_GAP;
        bool selected = (i == selectedMenu);

        if (selected) {
            canvas.fillRect(8, y, 304, MENU_ITEM_H, THEME_SELECT);
            canvas.setTextColor(THEME_SELTEXT, THEME_SELECT);
        } else {
            canvas.fillRect(8, y, 304, MENU_ITEM_H, THEME_PANEL);
            canvas.setTextColor(THEME_TEXT, THEME_PANEL);
        }

        drawMenuIcon(canvas, i, 16, y + 5);

        canvas.setTextSize(2);
        canvas.setCursor(48, y + 9);
        canvas.print(MENU_ITEMS[i]);
    }
}

void drawMenuScreen(bool animate = true) {
    drawHeader("CANON");

    if (selectedMenu < menuScroll) menuScroll = selectedMenu;
    if (selectedMenu >= menuScroll + MENU_VISIBLE) menuScroll = selectedMenu - MENU_VISIBLE + 1;

    drawFooter("UP:A", "OPEN:B", "DOWN:C");
    if (animate) slideInContent(drawMenuContent); else pushContent(drawMenuContent);
}

// ---------------------------------------------------------------------
// File browser (SPIFFS - internal flash, no SD card)
// ---------------------------------------------------------------------

void refreshFileList() {
    fileCount = 0;

    File dir = SPIFFS.open("/");
    if (!dir) return;

    File entry = dir.openNextFile();
    while (entry && fileCount < 64) {
        if (!entry.isDirectory()) {
            String name = String(entry.name());
            if (name.startsWith("/")) name = name.substring(1);
            fileNames[fileCount++] = name;
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();

    if (selectedFile >= fileCount) selectedFile = max(0, fileCount - 1);
}

void drawFileContent(int yShift) {
    canvas.fillSprite(THEME_BG);

    if (fileCount == 0) {
        canvas.setTextColor(THEME_TEXT, THEME_BG);
        canvas.setTextSize(2);
        canvas.setCursor(20, 90 - CANVAS_TOP - yShift);
        canvas.print("No files yet.");
        canvas.setCursor(20, 120 - CANVAS_TOP - yShift);
        canvas.print("Plug into PC and run");
        canvas.setCursor(20, 145 - CANVAS_TOP - yShift);
        canvas.print("uploader/upload.py");
    } else {
        for (int i = 0; i < VISIBLE_ROWS; i++) {
            int idx = scrollOffset + i;
            if (idx >= fileCount) break;

            int y = (LIST_TOP - CANVAS_TOP - yShift) + i * ROW_HEIGHT;
            bool selected = (idx == selectedFile);

            if (selected) {
                canvas.fillRect(8, y, 304, ROW_HEIGHT - 2, THEME_SELECT);
                canvas.setTextColor(THEME_SELTEXT, THEME_SELECT);
            } else {
                canvas.fillRect(8, y, 304, ROW_HEIGHT - 2, THEME_PANEL);
                canvas.setTextColor(THEME_TEXT, THEME_PANEL);
            }
            canvas.setTextSize(2);
            canvas.setCursor(16, y + 3);

            String name = fileNames[idx];
            if (name.length() > 28) name = name.substring(0, 25) + "...";
            canvas.print(name);
        }
    }
}

void drawFileScreen(bool animate = true) {
    drawHeader("CANON - Files");

    if (fileCount > 0) {
        if (selectedFile < scrollOffset) scrollOffset = selectedFile;
        if (selectedFile >= scrollOffset + VISIBLE_ROWS) scrollOffset = selectedFile - VISIBLE_ROWS + 1;
    }

    drawFooter("UP:A", "BACK:B", "DOWN:C");
    if (animate) slideInContent(drawFileContent); else pushContent(drawFileContent);
}

// ---------------------------------------------------------------------
// Pet screen
// ---------------------------------------------------------------------

void drawPetContent(int yShift) {
    canvas.fillSprite(THEME_BG);

    canvas.fillRect(20, 40 - CANVAS_TOP - yShift, 280, 150, THEME_PANEL);

    canvas.setTextColor(THEME_TEXT, THEME_PANEL);
    canvas.setTextSize(1);
    canvas.setCursor(30, 172 - CANVAS_TOP - yShift);
    canvas.print("Sage Mode toad companion - always watching");
    canvas.setCursor(30, 184 - CANVAS_TOP - yShift);
    canvas.print("your files for you.");
}

void drawPetScreen() {
    drawHeader("Kawazu Kumite");
    drawFooter("", "BACK:B", "");
    slideInContent(drawPetContent);
    frog.draw(160, 110);
}

// ---------------------------------------------------------------------
// Rasengan screen
// ---------------------------------------------------------------------

void drawRasenganContent(int yShift) {
    canvas.fillSprite(THEME_BG);
    canvas.fillRect(20, 40 - CANVAS_TOP - yShift, 280, 150, THEME_PANEL);
}

void drawRasenganScreen() {
    drawHeader("Rasengan");
    drawFooter("", "BACK:B", "");
    slideInContent(drawRasenganContent);
    rasengan.draw(160, 115, 60);
}

// ---------------------------------------------------------------------
// Getsuga Tensho screen (Bleach)
// ---------------------------------------------------------------------

void drawGetsugaContent(int yShift) {
    canvas.fillSprite(THEME_BG);
    canvas.fillRect(20, 40 - CANVAS_TOP - yShift, 280, 150, THEME_PANEL);
}

void drawGetsugaScreen() {
    drawHeader("Getsuga Tensho");
    drawFooter("", "BACK:B", "");
    slideInContent(drawGetsugaContent);
    getsuga.draw(160, 115, 55);
}

// ---------------------------------------------------------------------
// Hollow Mask screen (Bleach)
// ---------------------------------------------------------------------

void drawHollowContent(int yShift) {
    canvas.fillSprite(THEME_BG);
    canvas.fillRect(20, 40 - CANVAS_TOP - yShift, 280, 150, THEME_PANEL);
}

void drawHollowScreen() {
    drawHeader("Hollow Mask");
    drawFooter("", "BACK:B", "");
    slideInContent(drawHollowContent);
    hollowMask.draw(160, 115);
}

// ---------------------------------------------------------------------
// WiFi captive portal screen
// ---------------------------------------------------------------------

void drawPortalContent(int yShift) {
    canvas.fillSprite(THEME_BG);

    canvas.fillRect(20, 40 - CANVAS_TOP - yShift, 280, 150, THEME_PANEL);
    canvas.setTextColor(THEME_TEXT, THEME_PANEL);
    canvas.setTextSize(2);

    canvas.setCursor(32, 56 - CANVAS_TOP - yShift);
    canvas.print("Network:");
    canvas.setCursor(32, 80 - CANVAS_TOP - yShift);
    canvas.print(PORTAL_AP_NAME);

    canvas.setCursor(32, 116 - CANVAS_TOP - yShift);
    canvas.print("Address:");
    canvas.setCursor(32, 140 - CANVAS_TOP - yShift);
    canvas.print(portal.getIP().toString());

    canvas.setCursor(32, 168 - CANVAS_TOP - yShift);
    canvas.printf("Logins captured: %d", portal.getCaptureCount());
}

void drawPortalScreen(bool animate = true) {
    drawHeader("WiFi Portal");
    drawFooter("", "STOP:B", "");
    if (animate) slideInContent(drawPortalContent); else pushContent(drawPortalContent);
}

// ---------------------------------------------------------------------
// WiFi scanner
// ---------------------------------------------------------------------

const int WIFI_SCAN_MAX = 20;
String wifiSSID[WIFI_SCAN_MAX];
int32_t wifiRSSI[WIFI_SCAN_MAX];
bool wifiSecure[WIFI_SCAN_MAX];
int wifiCount = 0;
int wifiSelected = 0;
int wifiScroll = 0;

void drawScanningContent(int yShift) {
    canvas.fillSprite(THEME_BG);
    canvas.setTextColor(THEME_TEXT, THEME_BG);
    canvas.setTextSize(2);
    canvas.setCursor(20, 100 - CANVAS_TOP - yShift);
    canvas.print("Scanning...");
}

void scanWifiNetworks() {
    drawHeader("WiFi Scanner");
    drawFooter("", "", "");
    pushContent(drawScanningContent);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks();
    wifiCount = 0;
    for (int i = 0; i < n && wifiCount < WIFI_SCAN_MAX; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) ssid = "(hidden)";
        wifiSSID[wifiCount] = ssid;
        wifiRSSI[wifiCount] = WiFi.RSSI(i);
        wifiSecure[wifiCount] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        wifiCount++;
    }
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);

    wifiSelected = 0;
    wifiScroll = 0;
}

void drawWifiScanContent(int yShift) {
    canvas.fillSprite(THEME_BG);

    if (wifiCount == 0) {
        canvas.setTextColor(THEME_TEXT, THEME_BG);
        canvas.setTextSize(2);
        canvas.setCursor(20, 100 - CANVAS_TOP - yShift);
        canvas.print("No networks found.");
    } else {
        for (int i = 0; i < VISIBLE_ROWS; i++) {
            int idx = wifiScroll + i;
            if (idx >= wifiCount) break;

            int y = (LIST_TOP - CANVAS_TOP - yShift) + i * ROW_HEIGHT;
            bool selected = (idx == wifiSelected);

            if (selected) {
                canvas.fillRect(8, y, 304, ROW_HEIGHT - 2, THEME_SELECT);
                canvas.setTextColor(THEME_SELTEXT, THEME_SELECT);
            } else {
                canvas.fillRect(8, y, 304, ROW_HEIGHT - 2, THEME_PANEL);
                canvas.setTextColor(THEME_TEXT, THEME_PANEL);
            }

            canvas.setTextSize(2);
            canvas.setCursor(16, y + 3);
            String name = wifiSSID[idx];
            if (name.length() > 16) name = name.substring(0, 13) + "...";
            canvas.printf("%s%-17s%4ddBm", wifiSecure[idx] ? "*" : " ", name.c_str(), wifiRSSI[idx]);
        }
    }
}

void drawWifiScanScreen(bool animate = true) {
    drawHeader("WiFi Scanner");

    if (wifiCount > 0) {
        if (wifiSelected < wifiScroll) wifiScroll = wifiSelected;
        if (wifiSelected >= wifiScroll + VISIBLE_ROWS) wifiScroll = wifiSelected - VISIBLE_ROWS + 1;
    }

    drawFooter("UP:A", "BACK:B", "DOWN:C");
    if (animate) slideInContent(drawWifiScanContent); else pushContent(drawWifiScanContent);
}

// ---------------------------------------------------------------------
// BLE scanner
// ---------------------------------------------------------------------

const int BLE_SCAN_MAX = 12;
String bleName[BLE_SCAN_MAX];
String bleAddr[BLE_SCAN_MAX];
int bleRSSI[BLE_SCAN_MAX];
int bleCount = 0;
int bleSelected = 0;
int bleScroll = 0;
bool bleInited = false;

void scanBleDevices() {
    drawHeader("BLE Scanner");
    drawFooter("", "", "");
    pushContent(drawScanningContent);

    if (!bleInited) {
        NimBLEDevice::init("");
        bleInited = true;
    }

    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setActiveScan(true);
    scan->start(3, false);
    NimBLEScanResults results = scan->getResults();

    bleCount = 0;
    int found = results.getCount();
    for (int i = 0; i < found && bleCount < BLE_SCAN_MAX; i++) {
        NimBLEAdvertisedDevice dev = results.getDevice(i);
        bleAddr[bleCount] = dev.getAddress().toString().c_str();
        bleName[bleCount] = dev.haveName() ? dev.getName().c_str() : "(no name)";
        bleRSSI[bleCount] = dev.getRSSI();
        bleCount++;
    }
    scan->clearResults();

    bleSelected = 0;
    bleScroll = 0;
}

void drawBleScanContent(int yShift) {
    canvas.fillSprite(THEME_BG);

    if (bleCount == 0) {
        canvas.setTextColor(THEME_TEXT, THEME_BG);
        canvas.setTextSize(2);
        canvas.setCursor(20, 100 - CANVAS_TOP - yShift);
        canvas.print("No BLE devices found.");
    } else {
        for (int i = 0; i < VISIBLE_ROWS; i++) {
            int idx = bleScroll + i;
            if (idx >= bleCount) break;

            int y = (LIST_TOP - CANVAS_TOP - yShift) + i * ROW_HEIGHT;
            bool selected = (idx == bleSelected);

            if (selected) {
                canvas.fillRect(8, y, 304, ROW_HEIGHT - 2, THEME_SELECT);
                canvas.setTextColor(THEME_SELTEXT, THEME_SELECT);
            } else {
                canvas.fillRect(8, y, 304, ROW_HEIGHT - 2, THEME_PANEL);
                canvas.setTextColor(THEME_TEXT, THEME_PANEL);
            }

            canvas.setTextSize(1);
            canvas.setCursor(16, y + 3);
            String name = bleName[idx];
            if (name.length() > 18) name = name.substring(0, 15) + "...";
            canvas.printf("%-18s %s", name.c_str(), bleAddr[idx].c_str());
            canvas.setCursor(16, y + 13);
            canvas.printf("RSSI: %d dBm", bleRSSI[idx]);
        }
    }
}

void drawBleScanScreen(bool animate = true) {
    drawHeader("BLE Scanner");

    if (bleCount > 0) {
        if (bleSelected < bleScroll) bleScroll = bleSelected;
        if (bleSelected >= bleScroll + VISIBLE_ROWS) bleScroll = bleSelected - VISIBLE_ROWS + 1;
    }

    drawFooter("UP:A", "BACK:B", "DOWN:C");
    if (animate) slideInContent(drawBleScanContent); else pushContent(drawBleScanContent);
}

// ---------------------------------------------------------------------
// IR remote (NEC protocol, bit-banged carrier on IR_PIN)
// ---------------------------------------------------------------------

#define IR_PIN 12

struct IrCode {
    const char* name;
    uint32_t code;
};

const IrCode IR_CODES[] = {
    {"Samsung TV Power", 0xE0E040BF},
    {"LG TV Power",      0x20DF10EF},
    {"Vizio TV Power",   0x20DF23DC},
    {"Generic NEC Power", 0x00FF629D},
    {"<- Back to Menu",  0},
};
const int IR_CODE_COUNT = sizeof(IR_CODES) / sizeof(IR_CODES[0]);
const int IR_BACK_INDEX = IR_CODE_COUNT - 1;
int irSelected = 0;
unsigned long irSentAt = 0;

void irMark(uint32_t durationUs) {
    unsigned long endTime = micros() + durationUs;
    while (micros() < endTime) {
        digitalWrite(IR_PIN, HIGH);
        delayMicroseconds(13);
        digitalWrite(IR_PIN, LOW);
        delayMicroseconds(11);
    }
}

void irSpace(uint32_t durationUs) {
    digitalWrite(IR_PIN, LOW);
    delayMicroseconds(durationUs);
}

void irSendNEC(uint32_t code) {
    pinMode(IR_PIN, OUTPUT);
    irMark(9000);
    irSpace(4500);
    for (int i = 31; i >= 0; i--) {
        irMark(560);
        irSpace((code & (1UL << i)) ? 1690 : 560);
    }
    irMark(560);
    digitalWrite(IR_PIN, LOW);
}

void drawIrContent(int yShift) {
    canvas.fillSprite(THEME_BG);

    canvas.fillRect(20, 40 - CANVAS_TOP - yShift, 280, 150, THEME_PANEL);
    canvas.setTextColor(THEME_TEXT, THEME_PANEL);
    canvas.setTextSize(2);
    canvas.setCursor(32, 50 - CANVAS_TOP - yShift);
    canvas.print("Send IR power code:");

    for (int i = 0; i < IR_CODE_COUNT; i++) {
        int y = (80 - CANVAS_TOP - yShift) + i * 24;
        bool selected = (i == irSelected);
        canvas.setTextColor(selected ? THEME_SELTEXT : THEME_TEXT, selected ? THEME_SELECT : THEME_PANEL);
        if (selected) canvas.fillRect(28, y - 2, 264, 22, THEME_SELECT);
        else canvas.fillRect(28, y - 2, 264, 22, THEME_PANEL);
        canvas.setCursor(36, y);
        canvas.print(IR_CODES[i].name);
    }

    if (millis() - irSentAt < 800) {
        canvas.setTextColor(THEME_ACCENT, THEME_PANEL);
        canvas.setCursor(32, 175 - CANVAS_TOP - yShift);
        canvas.print("Sent!");
    }
}

void drawIrScreen(bool animate = true) {
    drawHeader("IR Remote");
    drawFooter("UP:A", "SELECT:B", "DOWN:C");
    if (animate) slideInContent(drawIrContent); else pushContent(drawIrContent);
}

// ---------------------------------------------------------------------
// I2C / Grove bus scanner
// ---------------------------------------------------------------------

const int I2C_SCAN_MAX = 16;
uint8_t i2cAddrs[I2C_SCAN_MAX];
int i2cCount = 0;

void scanI2C() {
    Wire.begin();
    i2cCount = 0;
    for (uint8_t addr = 1; addr < 127 && i2cCount < I2C_SCAN_MAX; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            i2cAddrs[i2cCount++] = addr;
        }
    }
}

void drawI2CContent(int yShift) {
    canvas.fillSprite(THEME_BG);

    canvas.fillRect(20, 40 - CANVAS_TOP - yShift, 280, 150, THEME_PANEL);
    canvas.setTextColor(THEME_TEXT, THEME_PANEL);
    canvas.setTextSize(2);

    if (i2cCount == 0) {
        canvas.setCursor(32, 56 - CANVAS_TOP - yShift);
        canvas.print("No I2C devices found");
        canvas.setCursor(32, 80 - CANVAS_TOP - yShift);
        canvas.print("on Grove port (21/22).");
    } else {
        canvas.setCursor(32, 50 - CANVAS_TOP - yShift);
        canvas.printf("%d device(s) found:", i2cCount);
        for (int i = 0; i < i2cCount && i < 6; i++) {
            canvas.setCursor(32, (76 - CANVAS_TOP - yShift) + i * 18);
            canvas.printf("0x%02X", i2cAddrs[i]);
        }
    }
}

void drawI2CScreen(bool animate = true) {
    drawHeader("I2C Scanner");
    drawFooter("BACK:A", "RESCAN:B", "");
    if (animate) slideInContent(drawI2CContent); else pushContent(drawI2CContent);
}

// ---------------------------------------------------------------------
// Serial upload protocol
//   MAGIC(4) + nameLen(1) + name(nameLen) + fileSize(4, LE) + data(fileSize)
//   Device replies "OK\n" or "ERR\n"
// ---------------------------------------------------------------------

bool readExact(uint8_t* buf, size_t len, uint32_t timeoutMs = 5000) {
    size_t got = 0;
    uint32_t start = millis();
    while (got < len) {
        if (Serial.available()) {
            got += Serial.readBytes(buf + got, len - got);
        } else if (millis() - start > timeoutMs) {
            return false;
        }
    }
    return true;
}

void handleUpload() {
    uint8_t nameLenByte;
    if (!readExact(&nameLenByte, 1)) return;

    char nameBuf[256] = {0};
    if (!readExact((uint8_t*)nameBuf, nameLenByte)) return;
    nameBuf[nameLenByte] = '\0';

    uint8_t sizeBuf[4];
    if (!readExact(sizeBuf, 4)) return;
    uint32_t fileSize = (uint32_t)sizeBuf[0] | ((uint32_t)sizeBuf[1] << 8) |
                         ((uint32_t)sizeBuf[2] << 16) | ((uint32_t)sizeBuf[3] << 24);

    String path = "/" + String(nameBuf);
    File f = SPIFFS.open(path, FILE_WRITE);
    if (!f) {
        Serial.print("ERR\n");
        return;
    }

    uint8_t chunk[512];
    uint32_t remaining = fileSize;
    bool ok = true;

    M5.Lcd.fillScreen(THEME_BG);
    drawHeader("Receiving file...");
    M5.Lcd.setTextColor(THEME_TEXT, THEME_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(16, 60);
    M5.Lcd.print(nameBuf);

    while (remaining > 0) {
        size_t want = remaining > sizeof(chunk) ? sizeof(chunk) : remaining;
        if (!readExact(chunk, want)) { ok = false; break; }
        f.write(chunk, want);
        remaining -= want;

        // Progress bar
        int pct = (int)(100 - (remaining * 100 / fileSize));
        M5.Lcd.fillRect(16, 100, 288, 20, THEME_PANEL);
        M5.Lcd.drawRect(16, 100, 288, 20, THEME_ACCENT);
        M5.Lcd.fillRect(18, 102, (288 - 4) * pct / 100, 16, THEME_SELECT);
        M5.Lcd.setCursor(16, 130);
        M5.Lcd.printf("%d%%   ", pct);
    }

    f.close();
    Serial.print(ok ? "OK\n" : "ERR\n");

    refreshFileList();
    state = STATE_FILES;
    drawFileScreen();
}

// ---------------------------------------------------------------------

void drawBootSplash() {
    M5.Lcd.fillScreen(THEME_BG);

    M5.Lcd.setTextColor(THEME_ACCENT, THEME_BG);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(18, 30);
    M5.Lcd.println("TAKE THIS");
    M5.Lcd.setCursor(18, 65);
    M5.Lcd.println("RASENGAN!");

    for (int i = 0; i < 40; i++) {
        rasengan.update();
        rasengan.draw(160, 160, 55);
        delay(30);
    }

    M5.Lcd.fillScreen(THEME_BG);
}

void setup() {
    M5.begin();
    M5.Power.begin();
    Serial.begin(115200);

    SPIFFS.begin(true); // format on first use - no SD card needed

    canvas.setColorDepth(16);
    canvas.createSprite(320, CANVAS_H);

    drawBootSplash();

    refreshFileList();
    drawMenuScreen();
}

void loop() {
    M5.update();

    // Check for an incoming file transfer
    if (Serial.available() >= MAGIC_LEN) {
        uint8_t magic[MAGIC_LEN];
        Serial.readBytes(magic, MAGIC_LEN);
        if (memcmp(magic, MAGIC, MAGIC_LEN) == 0) {
            handleUpload();
            return;
        }
    }

    switch (state) {
        case STATE_MENU:
            if (M5.BtnA.wasPressed()) {
                if (selectedMenu > 0) selectedMenu--;
                drawMenuScreen(false);
            }
            if (M5.BtnC.wasPressed()) {
                if (selectedMenu < MENU_COUNT - 1) selectedMenu++;
                drawMenuScreen(false);
            }
            if (M5.BtnB.wasPressed()) {
                switch (selectedMenu) {
                    case 0:
                        refreshFileList();
                        state = STATE_FILES;
                        drawFileScreen();
                        break;
                    case 1:
                        state = STATE_PET;
                        drawPetScreen();
                        break;
                    case 2:
                        state = STATE_RASENGAN;
                        drawRasenganScreen();
                        break;
                    case 3:
                        state = STATE_GETSUGA;
                        drawGetsugaScreen();
                        break;
                    case 4:
                        state = STATE_HOLLOW;
                        drawHollowScreen();
                        break;
                    case 5:
                        portal.begin(PORTAL_AP_NAME);
                        lastPortalCaptures = -1;
                        state = STATE_PORTAL;
                        drawPortalScreen();
                        break;
                    case 6:
                        scanWifiNetworks();
                        state = STATE_WIFI_SCAN;
                        drawWifiScanScreen();
                        break;
                    case 7:
                        scanBleDevices();
                        state = STATE_BLE_SCAN;
                        drawBleScanScreen();
                        break;
                    case 8:
                        irSelected = 0;
                        irSentAt = 0;
                        state = STATE_IR;
                        drawIrScreen();
                        break;
                    case 9:
                        scanI2C();
                        state = STATE_I2C;
                        drawI2CScreen();
                        break;
                }
            }
            break;

        case STATE_FILES:
            if (M5.BtnA.wasPressed()) {
                if (selectedFile > 0) selectedFile--;
                drawFileScreen(false);
            }
            if (M5.BtnC.wasPressed()) {
                if (selectedFile < fileCount - 1) selectedFile++;
                drawFileScreen(false);
            }
            if (M5.BtnB.wasPressed()) {
                state = STATE_MENU;
                drawMenuScreen();
            }
            break;

        case STATE_PET:
            if (M5.BtnB.wasPressed()) {
                state = STATE_MENU;
                drawMenuScreen();
            }
            frog.update();
            M5.Lcd.fillRect(21, 41, 278, 148, THEME_PANEL);
            frog.draw(160, 110);
            delay(16);
            break;

        case STATE_RASENGAN:
            if (M5.BtnB.wasPressed()) {
                state = STATE_MENU;
                drawMenuScreen();
            }
            rasengan.update();
            M5.Lcd.fillRect(21, 41, 278, 148, THEME_PANEL);
            rasengan.draw(160, 115, 60);
            delay(16);
            break;

        case STATE_GETSUGA:
            if (M5.BtnB.wasPressed()) {
                state = STATE_MENU;
                drawMenuScreen();
            }
            getsuga.update();
            M5.Lcd.fillRect(21, 41, 278, 148, THEME_PANEL);
            getsuga.draw(160, 115, 55);
            delay(16);
            break;

        case STATE_HOLLOW:
            if (M5.BtnB.wasPressed()) {
                state = STATE_MENU;
                drawMenuScreen();
            }
            hollowMask.update();
            M5.Lcd.fillRect(21, 41, 278, 148, THEME_PANEL);
            hollowMask.draw(160, 115);
            delay(16);
            break;

        case STATE_WIFI_SCAN:
            if (M5.BtnA.wasPressed()) {
                if (wifiSelected > 0) wifiSelected--;
                drawWifiScanScreen(false);
            }
            if (M5.BtnC.wasPressed()) {
                if (wifiSelected < wifiCount - 1) wifiSelected++;
                drawWifiScanScreen(false);
            }
            if (M5.BtnB.wasPressed()) {
                state = STATE_MENU;
                drawMenuScreen();
            }
            break;

        case STATE_BLE_SCAN:
            if (M5.BtnA.wasPressed()) {
                if (bleSelected > 0) bleSelected--;
                drawBleScanScreen(false);
            }
            if (M5.BtnC.wasPressed()) {
                if (bleSelected < bleCount - 1) bleSelected++;
                drawBleScanScreen(false);
            }
            if (M5.BtnB.wasPressed()) {
                state = STATE_MENU;
                drawMenuScreen();
            }
            break;

        case STATE_IR:
            if (M5.BtnA.wasPressed()) {
                if (irSelected > 0) irSelected--;
                drawIrScreen(false);
            }
            if (M5.BtnC.wasPressed()) {
                if (irSelected < IR_CODE_COUNT - 1) irSelected++;
                drawIrScreen(false);
            }
            if (M5.BtnB.wasPressed()) {
                if (irSelected == IR_BACK_INDEX) {
                    state = STATE_MENU;
                    drawMenuScreen();
                } else {
                    irSendNEC(IR_CODES[irSelected].code);
                    irSentAt = millis();
                    drawIrScreen(false);
                }
            }
            if (millis() - irSentAt > 800 && millis() - irSentAt < 850) {
                drawIrScreen(false);
            }
            break;

        case STATE_I2C:
            if (M5.BtnA.wasPressed()) {
                state = STATE_MENU;
                drawMenuScreen();
            }
            if (M5.BtnB.wasPressed()) {
                scanI2C();
                drawI2CScreen(false);
            }
            break;

        case STATE_PORTAL:
            if (M5.BtnB.wasPressed()) {
                portal.end();
                refreshFileList();
                state = STATE_MENU;
                drawMenuScreen();
                break;
            }
            portal.handle();
            if (portal.getCaptureCount() != lastPortalCaptures) {
                lastPortalCaptures = portal.getCaptureCount();
                drawPortalScreen(false);
            }
            break;
    }
}
