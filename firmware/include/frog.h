#pragma once
#include <M5Stack.h>
#include "theme.h"

// "Kawazu Kumite" - a small sage-mode frog companion.
// Drawn procedurally so no image assets are needed.
class Frog {
public:
    void update() {
        uint32_t now = millis();

        // Breathing bob (slow vertical sine-ish motion)
        bobOffset = (sin(now / 600.0) * 4.0);

        // Blink every ~3.5s, blink lasts 150ms
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

    // Draw the frog centered at (cx, cy)
    void draw(int cx, int cy) {
        int by = cy + (int)bobOffset;

        // Body
        M5.Lcd.fillEllipse(cx, by + 10, 38, 26, FROG_BODY);
        // Belly
        M5.Lcd.fillEllipse(cx, by + 18, 24, 14, FROG_BELLY);

        // Sage-mode markings around eyes (orange rings)
        M5.Lcd.fillCircle(cx - 18, by - 14, 13, FROG_EYE_RING);
        M5.Lcd.fillCircle(cx + 18, by - 14, 13, FROG_EYE_RING);

        // Eye whites (or closed line if blinking)
        if (blinking) {
            M5.Lcd.drawFastHLine(cx - 25, by - 14, 14, THEME_TEXT);
            M5.Lcd.drawFastHLine(cx + 11, by - 14, 14, THEME_TEXT);
        } else {
            M5.Lcd.fillCircle(cx - 18, by - 14, 9, THEME_HILIGHT);
            M5.Lcd.fillCircle(cx + 18, by - 14, 9, THEME_HILIGHT);
            // Pupils
            M5.Lcd.fillCircle(cx - 18, by - 14, 4, THEME_TEXT);
            M5.Lcd.fillCircle(cx + 18, by - 14, 4, THEME_TEXT);
        }

        // Front legs / hands resting on belly
        M5.Lcd.fillCircle(cx - 20, by + 26, 7, FROG_BODY);
        M5.Lcd.fillCircle(cx + 20, by + 26, 7, FROG_BODY);

        // Mouth
        M5.Lcd.drawArc(cx, by + 6, 14, 12, 30, 150, THEME_TEXT, FROG_BODY);

        // Tongue flick
        if (tongueOut) {
            M5.Lcd.fillRoundRect(cx - 4, by + 14, 8, 16, 3, FROG_MARK);
        }

        // Sage mode marking on forehead (a small swirl dot)
        M5.Lcd.fillCircle(cx, by - 30, 4, FROG_MARK);
    }

    const char* name() const { return "Kawazu Kumite"; }

private:
    float bobOffset = 0;
    bool blinking = false;
    bool tongueOut = false;
    uint32_t lastBlink = 0;
    uint32_t lastTongue = 0;
};
