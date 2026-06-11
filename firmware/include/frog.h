#pragma once
#include <M5Stack.h>
#include "theme.h"

// "Kawazu Kumite" - a pixel-art sage-mode toad companion, drawn in the
// blocky orange-on-black "Bruce" style. The sprite is a fixed 16x14 grid
// of color codes, scaled up into chunky pixels.
//
// Legend:
//   . = transparent (background shows through)
//   D = dark brown outline / eyebrows / mouth
//   O = toad orange (body)
//   E = eyebrow (dark)
//   W = eye white
//   B = pupil
//   V = vest strap (blue-grey)
//   C = cream belly
//   M = mouth (dark)
class Frog {
public:
    static const int COLS = 16;
    static const int ROWS = 14;
    static const int PIXEL = 9; // on-screen size of one sprite "pixel"

    void update() {
        uint32_t now = millis();

        // Slow vertical bob
        bobOffset = (int)(sin(now / 600.0) * 3.0);

        // Blink every ~3.5s, lasts 150ms
        if (!blinking && now - lastBlink > 3500) {
            blinking = true;
            lastBlink = now;
        }
        if (blinking && now - lastBlink > 150) {
            blinking = false;
            lastBlink = now;
        }

        // Tongue flick every ~6s
        if (!tongueOut && now - lastTongue > 6000) {
            tongueOut = true;
            lastTongue = now;
        }
        if (tongueOut && now - lastTongue > 300) {
            tongueOut = false;
            lastTongue = now;
        }
    }

    // Draw the sprite centered at (cx, cy)
    void draw(int cx, int cy) {
        static const char* SPRITE[ROWS] = {
            ".....DDDDDD.....",
            "....DOODDOOD....",
            "...DEEODDOEED...",
            "...DWBODDOBWD...",
            "...DOOODDOOOD...",
            "..DOVOODDOOVOD..",
            ".DOOVOODDOOVOOD.",
            "DOOOOOOMMOOOOOOD",
            "DOOCCCCCCCCCCOOD",
            "DOCCCCCCCCCCCCOD",
            "DOVVVVVVVVVVVVOD",
            "DOOOOOOOOOOOOOOD",
            "DDOOOOOOOOOOOODD",
            ".DDDDDDDDDDDDDD.",
        };

        int originX = cx - (COLS * PIXEL) / 2;
        int originY = cy - (ROWS * PIXEL) / 2 + bobOffset;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                char cell = SPRITE[r][c];
                if (cell == '.') continue;

                // Eyes are on row 3; close them when blinking
                if (blinking && r == 3 && (cell == 'W' || cell == 'B')) {
                    cell = 'D';
                }

                M5.Lcd.fillRect(originX + c * PIXEL, originY + r * PIXEL,
                                 PIXEL, PIXEL, colorFor(cell));
            }
        }

        // Tongue flick below the mouth
        if (tongueOut) {
            int tx = originX + 7 * PIXEL;
            int ty = originY + 8 * PIXEL;
            M5.Lcd.fillRect(tx, ty, PIXEL * 2, PIXEL * 2, FROG_TONGUE);
        }
    }

    const char* name() const { return "Kawazu Kumite"; }

private:
    static uint16_t colorFor(char cell) {
        switch (cell) {
            case 'D': return FROG_DARK;
            case 'O': return FROG_BODY;
            case 'E': return FROG_DARK;
            case 'W': return FROG_EYE;
            case 'B': return FROG_PUPIL;
            case 'V': return FROG_VEST;
            case 'C': return FROG_BELLY;
            case 'M': return FROG_DARK;
            default:  return THEME_BG;
        }
    }

    int bobOffset = 0;
    bool blinking = false;
    bool tongueOut = false;
    uint32_t lastBlink = 0;
    uint32_t lastTongue = 0;
};
