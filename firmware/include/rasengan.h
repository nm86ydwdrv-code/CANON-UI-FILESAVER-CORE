#pragma once
#include <M5Stack.h>
#include "theme.h"

// A pixelated, animated Rasengan - drawn as chunky rotating blocks
// spiraling out of a bright core, like a low-res sprite.
class Rasengan {
public:
    static const int PIXEL = 4;

    void update() {
        phase += 0.18f;
        if (phase > TWO_PI) phase -= TWO_PI;
    }

    // Draw centered at (cx, cy) with the given outer radius
    void draw(int cx, int cy, int radius) {
        // Pixelated core
        drawPixelCircle(cx, cy, radius / 3, RASEN_CORE);

        // Three swirling arms spiraling outward
        for (int arm = 0; arm < 3; arm++) {
            float armOffset = arm * (TWO_PI / 3.0f);
            for (float t = 0.15f; t <= 1.0f; t += 0.08f) {
                float r = t * radius;
                float theta = phase + armOffset + t * 6.0f;
                int x = cx + (int)(cosf(theta) * r);
                int y = cy + (int)(sinf(theta) * r);

                uint16_t color = (t < 0.55f) ? RASEN_MID : RASEN_OUTER;
                drawBlock(x, y, color);
            }
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
