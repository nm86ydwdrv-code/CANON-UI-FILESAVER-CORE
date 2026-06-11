#pragma once
#include <M5Stack.h>
#include "theme.h"

// Bleach-themed pixel art: Getsuga Tensho slash and a Hollow mask.

// Animated pixelated Getsuga Tensho - a black crescent of reiatsu with a
// pale cyan glow and orbiting particles.
class GetsugaTensho {
public:
    static const int PIXEL = 4;

    void update() {
        phase += 0.12f;
        if (phase > TWO_PI) phase -= TWO_PI;
    }

    void draw(int cx, int cy, int radius) {
        // Crescent body: a dark circle with a panel-colored circle cut
        // out of it, breathing slightly with phase.
        int offsetX = (int)(radius * (0.55f + 0.1f * sinf(phase)));

        drawPixelCircle(cx, cy, radius, GETSUGA_DARK);
        drawPixelCircle(cx + offsetX, cy, radius, THEME_PANEL);

        // Glow edge along the crescent's outer rim
        for (float a = -1.2f; a <= 1.2f; a += 0.15f) {
            int x = cx + (int)(cosf(a) * radius);
            int y = cy + (int)(sinf(a) * radius);
            drawBlock(x, y, GETSUGA_GLOW);
        }

        // Orbiting reiatsu particles
        for (int i = 0; i < 6; i++) {
            float a = phase + i * (TWO_PI / 6.0f);
            int px = cx + (int)(cosf(a) * (radius + 14));
            int py = cy + (int)(sinf(a) * (radius + 14) * 0.6f);
            drawBlock(px, py, GETSUGA_GLOW);
        }
    }

private:
    void drawBlock(int x, int y, uint16_t color) {
        int bx = (x / PIXEL) * PIXEL;
        int by = (y / PIXEL) * PIXEL;
        M5.Lcd.fillRect(bx, by, PIXEL, PIXEL, color);
    }

    void drawPixelCircle(int cx, int cy, int radius, uint16_t color) {
        for (int y = -radius; y <= radius; y += PIXEL) {
            for (int x = -radius; x <= radius; x += PIXEL) {
                if (x * x + y * y <= radius * radius) {
                    drawBlock(cx + x, cy + y, color);
                }
            }
        }
    }

    float phase = 0.0f;
};

// A pixel-art Hollow mask with pulsing red markings.
class HollowMask {
public:
    static const int COLS = 16;
    static const int ROWS = 14;
    static const int PIXEL = 9;

    void update() {
        glowPhase += 0.08f;
        if (glowPhase > TWO_PI) glowPhase -= TWO_PI;
    }

    void draw(int cx, int cy) {
        static const char* SPRITE[ROWS] = {
            ".....DDDDDD.....",
            "....DWWDDWWD....",
            "...DWRWDDWRWD...",
            "...DWBWDDWBWD...",
            "...DWRWDDWRWD...",
            "..DWWWWDDWWWWD..",
            ".DWWWWWDDWWWWWD.",
            "DWBWBWBWWBWBWBWD",
            "DWWWWWWDDWWWWWWD",
            ".DWWWWWDDWWWWWD.",
            "..DWWWWDDWWWWD..",
            "...DWWD..DWWD...",
            "....DWD..DWD....",
            ".....DD..DD.....",
        };

        // Pulse the red markings between bright and dark red
        float pulse = (sinf(glowPhase) + 1.0f) / 2.0f; // 0..1
        uint16_t redNow = blend(0x7800, HOLLOW_RED, pulse);

        int originX = cx - (COLS * PIXEL) / 2;
        int originY = cy - (ROWS * PIXEL) / 2;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                char cell = SPRITE[r][c];
                if (cell == '.') continue;

                uint16_t color;
                switch (cell) {
                    case 'D': color = HOLLOW_DARK; break;
                    case 'W': color = HOLLOW_WHITE; break;
                    case 'B': color = HOLLOW_DARK; break;
                    case 'R': color = redNow; break;
                    default:  color = HOLLOW_DARK; break;
                }

                M5.Lcd.fillRect(originX + c * PIXEL, originY + r * PIXEL,
                                 PIXEL, PIXEL, color);
            }
        }
    }

private:
    static uint16_t blend(uint16_t a, uint16_t b, float t) {
        int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
        int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
        int r = ar + (int)((br - ar) * t);
        int g = ag + (int)((bg - ag) * t);
        int bl = ab + (int)((bb - ab) * t);
        return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (bl & 0x1F);
    }

    float glowPhase = 0.0f;
};
