#include <M5Stack.h>
#include <SPIFFS.h>
#include "theme.h"
#include "frog.h"
#include "rasengan.h"
#include "bleach.h"

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
    STATE_HOLLOW
};
AppState state = STATE_MENU;

Frog frog;
Rasengan rasengan;
GetsugaTensho getsuga;
HollowMask hollowMask;

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
};
const int MENU_COUNT = 5;
const int MENU_ITEM_H = 32;
const int MENU_ITEM_GAP = 36;
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

void drawIcon(int x, int y, const char* rows[8], uint16_t* palette, int pixel = 4) {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char ch = rows[r][c];
            if (ch == '.') continue;
            uint16_t color = palette[ch - '0'];
            M5.Lcd.fillRect(x + c * pixel, y + r * pixel, pixel, pixel, color);
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
// Menu screen
// ---------------------------------------------------------------------

void drawMenuScreen() {
    M5.Lcd.fillScreen(THEME_BG);
    drawHeader("CANON");

    for (int i = 0; i < MENU_COUNT; i++) {
        int y = LIST_TOP + i * MENU_ITEM_GAP;
        bool selected = (i == selectedMenu);

        if (selected) {
            M5.Lcd.fillRect(8, y, 304, MENU_ITEM_H, THEME_SELECT);
            M5.Lcd.setTextColor(THEME_SELTEXT, THEME_SELECT);
        } else {
            M5.Lcd.fillRect(8, y, 304, MENU_ITEM_H, THEME_PANEL);
            M5.Lcd.setTextColor(THEME_TEXT, THEME_PANEL);
        }

        switch (i) {
            case 0: drawIcon(16, y, ICON_FOLDER, PALETTE_FOLDER); break;
            case 1: drawIcon(16, y, ICON_FROG, PALETTE_FROG); break;
            case 2: drawIcon(16, y, ICON_RASEN, PALETTE_RASEN); break;
            case 3: drawIcon(16, y, ICON_GETSUGA, PALETTE_GETSUGA); break;
            case 4: drawIcon(16, y, ICON_HOLLOW, PALETTE_HOLLOW); break;
        }

        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(56, y + 8);
        M5.Lcd.print(MENU_ITEMS[i]);
    }

    drawFooter("UP:A", "OPEN:B", "DOWN:C");
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

void drawFileScreen() {
    M5.Lcd.fillScreen(THEME_BG);
    drawHeader("CANON - Files");

    if (fileCount == 0) {
        M5.Lcd.setTextColor(THEME_TEXT, THEME_BG);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(20, 90);
        M5.Lcd.print("No files yet.");
        M5.Lcd.setCursor(20, 120);
        M5.Lcd.print("Plug into PC and run");
        M5.Lcd.setCursor(20, 145);
        M5.Lcd.print("uploader/upload.py");
    } else {
        if (selectedFile < scrollOffset) scrollOffset = selectedFile;
        if (selectedFile >= scrollOffset + VISIBLE_ROWS) scrollOffset = selectedFile - VISIBLE_ROWS + 1;

        for (int i = 0; i < VISIBLE_ROWS; i++) {
            int idx = scrollOffset + i;
            if (idx >= fileCount) break;

            int y = LIST_TOP + i * ROW_HEIGHT;
            bool selected = (idx == selectedFile);

            if (selected) {
                M5.Lcd.fillRect(8, y, 304, ROW_HEIGHT - 2, THEME_SELECT);
                M5.Lcd.setTextColor(THEME_SELTEXT, THEME_SELECT);
            } else {
                M5.Lcd.fillRect(8, y, 304, ROW_HEIGHT - 2, THEME_PANEL);
                M5.Lcd.setTextColor(THEME_TEXT, THEME_PANEL);
            }
            M5.Lcd.setTextSize(2);
            M5.Lcd.setCursor(16, y + 3);

            String name = fileNames[idx];
            if (name.length() > 28) name = name.substring(0, 25) + "...";
            M5.Lcd.print(name);
        }
    }

    drawFooter("UP:A", "BACK:B", "DOWN:C");
}

// ---------------------------------------------------------------------
// Pet screen
// ---------------------------------------------------------------------

void drawPetScreen() {
    M5.Lcd.fillScreen(THEME_BG);
    drawHeader("Kawazu Kumite");

    M5.Lcd.fillRect(20, 40, 280, 150, THEME_PANEL);
    frog.draw(160, 110);

    M5.Lcd.setTextColor(THEME_TEXT, THEME_PANEL);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(30, 172);
    M5.Lcd.print("Sage Mode toad companion - always watching");
    M5.Lcd.setCursor(30, 184);
    M5.Lcd.print("your files for you.");

    drawFooter("", "BACK:B", "");
}

// ---------------------------------------------------------------------
// Rasengan screen
// ---------------------------------------------------------------------

void drawRasenganScreen() {
    M5.Lcd.fillScreen(THEME_BG);
    drawHeader("Rasengan");
    M5.Lcd.fillRect(20, 40, 280, 150, THEME_PANEL);
    rasengan.draw(160, 115, 60);
    drawFooter("", "BACK:B", "");
}

// ---------------------------------------------------------------------
// Getsuga Tensho screen (Bleach)
// ---------------------------------------------------------------------

void drawGetsugaScreen() {
    M5.Lcd.fillScreen(THEME_BG);
    drawHeader("Getsuga Tensho");
    M5.Lcd.fillRect(20, 40, 280, 150, THEME_PANEL);
    getsuga.draw(160, 115, 55);
    drawFooter("", "BACK:B", "");
}

// ---------------------------------------------------------------------
// Hollow Mask screen (Bleach)
// ---------------------------------------------------------------------

void drawHollowScreen() {
    M5.Lcd.fillScreen(THEME_BG);
    drawHeader("Hollow Mask");
    M5.Lcd.fillRect(20, 40, 280, 150, THEME_PANEL);
    hollowMask.draw(160, 115);
    drawFooter("", "BACK:B", "");
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

void setup() {
    M5.begin();
    M5.Power.begin();
    Serial.begin(115200);

    SPIFFS.begin(true); // format on first use - no SD card needed

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
                drawMenuScreen();
            }
            if (M5.BtnC.wasPressed()) {
                if (selectedMenu < MENU_COUNT - 1) selectedMenu++;
                drawMenuScreen();
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
                }
            }
            break;

        case STATE_FILES:
            if (M5.BtnA.wasPressed()) {
                if (selectedFile > 0) selectedFile--;
                drawFileScreen();
            }
            if (M5.BtnC.wasPressed()) {
                if (selectedFile < fileCount - 1) selectedFile++;
                drawFileScreen();
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
    }
}
