#include <M5Stack.h>
#include <SD.h>
#include "theme.h"
#include "frog.h"

// ---------------------------------------------------------------------
// CANON - M5Stack Core firmware
//   - Light-blue, Flipper-Zero-style file browser for the SD card
//   - Files are pushed from a PC over USB serial (see uploader/upload.py)
//   - A small sage-mode frog companion, Kawazu Kumite, lives on its own screen
// ---------------------------------------------------------------------

#define UPLOAD_DIR "/uploads"
#define MAGIC_LEN 4
const char MAGIC[MAGIC_LEN] = {'K', 'K', 'U', '1'};

enum AppState { STATE_FILES, STATE_PET };
AppState state = STATE_FILES;

Frog frog;

// File list state
String fileNames[64];
int fileCount = 0;
int selectedIndex = 0;
int scrollOffset = 0;
const int VISIBLE_ROWS = 7;
const int ROW_HEIGHT = 24;
const int LIST_TOP = 30;

bool sdReady = false;

// ---------------------------------------------------------------------
// UI drawing
// ---------------------------------------------------------------------

void drawHeader(const char* title) {
    M5.Lcd.fillRect(0, 0, 320, 24, THEME_ACCENT);
    M5.Lcd.setTextColor(THEME_HILIGHT, THEME_ACCENT);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(8, 4);
    M5.Lcd.print(title);
}

void drawFooter(const char* a, const char* b, const char* c) {
    M5.Lcd.fillRect(0, 216, 320, 24, THEME_ACCENT);
    M5.Lcd.setTextColor(THEME_HILIGHT, THEME_ACCENT);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(8, 220);
    M5.Lcd.print(a);
    M5.Lcd.setCursor(120, 220);
    M5.Lcd.print(b);
    M5.Lcd.setCursor(232, 220);
    M5.Lcd.print(c);
}

void refreshFileList() {
    fileCount = 0;
    if (!sdReady) return;

    File dir = SD.open(UPLOAD_DIR);
    if (!dir) return;

    File entry = dir.openNextFile();
    while (entry && fileCount < 64) {
        if (!entry.isDirectory()) {
            fileNames[fileCount++] = String(entry.name());
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();

    if (selectedIndex >= fileCount) selectedIndex = max(0, fileCount - 1);
}

void drawFileScreen() {
    M5.Lcd.fillScreen(THEME_BG);
    drawHeader("CANON - Files");

    if (!sdReady) {
        M5.Lcd.setTextColor(THEME_TEXT, THEME_BG);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(20, 100);
        M5.Lcd.print("No SD card detected");
    } else if (fileCount == 0) {
        M5.Lcd.setTextColor(THEME_TEXT, THEME_BG);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(20, 100);
        M5.Lcd.print("No files yet.");
        M5.Lcd.setCursor(20, 130);
        M5.Lcd.print("Plug into PC and run");
        M5.Lcd.setCursor(20, 155);
        M5.Lcd.print("uploader/upload.py");
    } else {
        // Clamp scroll so selection is visible
        if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
        if (selectedIndex >= scrollOffset + VISIBLE_ROWS) scrollOffset = selectedIndex - VISIBLE_ROWS + 1;

        for (int i = 0; i < VISIBLE_ROWS; i++) {
            int idx = scrollOffset + i;
            if (idx >= fileCount) break;

            int y = LIST_TOP + i * ROW_HEIGHT;
            bool selected = (idx == selectedIndex);

            M5.Lcd.fillRoundRect(8, y, 304, ROW_HEIGHT - 2, 4,
                                  selected ? THEME_SELECT : THEME_PANEL);
            M5.Lcd.setTextColor(selected ? THEME_HILIGHT : THEME_TEXT,
                                 selected ? THEME_SELECT : THEME_PANEL);
            M5.Lcd.setTextSize(2);
            M5.Lcd.setCursor(16, y + 3);

            String name = fileNames[idx];
            if (name.length() > 28) name = name.substring(0, 25) + "...";
            M5.Lcd.print(name);
        }
    }

    drawFooter("UP:A", "PET:B", "DOWN:C");
}

void drawPetScreen() {
    M5.Lcd.fillScreen(THEME_BG);
    drawHeader("Kawazu Kumite");

    M5.Lcd.fillRoundRect(20, 40, 280, 150, 8, THEME_PANEL);
    frog.draw(160, 120);

    M5.Lcd.setTextColor(THEME_TEXT, THEME_PANEL);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(30, 165);
    M5.Lcd.print("Sage Mode frog companion - always watching");
    M5.Lcd.setCursor(30, 178);
    M5.Lcd.print("your files for you.");

    drawFooter("", "FILES:B", "");
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

    if (!sdReady) {
        Serial.print("ERR\n");
        return;
    }

    if (!SD.exists(UPLOAD_DIR)) SD.mkdir(UPLOAD_DIR);

    String path = String(UPLOAD_DIR) + "/" + String(nameBuf);
    File f = SD.open(path, FILE_WRITE);
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
        M5.Lcd.fillRect(16, 100, 288 * pct / 100, 20, THEME_SELECT);
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

    sdReady = SD.begin();
    if (sdReady && !SD.exists(UPLOAD_DIR)) SD.mkdir(UPLOAD_DIR);

    refreshFileList();
    drawFileScreen();
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

    if (state == STATE_FILES) {
        if (M5.BtnA.wasPressed()) {
            if (selectedIndex > 0) selectedIndex--;
            drawFileScreen();
        }
        if (M5.BtnC.wasPressed()) {
            if (selectedIndex < fileCount - 1) selectedIndex++;
            drawFileScreen();
        }
        if (M5.BtnB.wasPressed()) {
            state = STATE_PET;
            drawPetScreen();
        }
    } else { // STATE_PET
        if (M5.BtnB.wasPressed()) {
            state = STATE_FILES;
            drawFileScreen();
        }
        frog.update();
        M5.Lcd.fillRoundRect(20, 40, 280, 150, 8, THEME_PANEL);
        frog.draw(160, 120);
        delay(16); // ~60fps redraw for the pet animation
    }
}
