#include "spinner.h"
#include <Arduino.h>

extern int BTN;
extern int readButton();

#define SCREEN_W 128
#define SCREEN_H  32

// ── Nothing Phone Bottle Glyph (neck up) ────────────────────────────────────────
static const unsigned char PROGMEM bottle_bmp[] = {
0x00,0x00,0x00,0x00, 0x00,0x03,0xe0,0x00, 0x00,0x00,0x00,0x00, 0x00,0x03,0xe0,0x00,
0x00,0x02,0x20,0x00, 0x00,0x02,0x20,0x00, 0x00,0x02,0x20,0x00, 0x00,0x02,0x20,0x00,
0x00,0x02,0x20,0x00, 0x00,0x02,0x20,0x00, 0x00,0x02,0x20,0x00, 0x00,0x0c,0x18,0x00,
0x00,0x10,0x04,0x00, 0x00,0x20,0x02,0x00, 0x00,0x20,0x02,0x00, 0x00,0x20,0x02,0x00,
0x00,0x20,0x02,0x00, 0x00,0x20,0x02,0x00, 0x00,0x20,0x02,0x00, 0x00,0x20,0x02,0x00,
0x00,0x20,0x02,0x00, 0x00,0x20,0x02,0x00, 0x00,0x2a,0xaa,0x00, 0x00,0x20,0x02,0x00,
0x00,0x25,0x52,0x00, 0x00,0x20,0x02,0x00, 0x00,0x2a,0xaa,0x00, 0x00,0x20,0x02,0x00,
0x00,0x25,0x52,0x00, 0x00,0x10,0x04,0x00, 0x00,0x0f,0xf8,0x00, 0x00,0x00,0x00,0x00
};

static const char *DIR_LABELS[8] = {
    "ONE","TWO","THREE","FOUR",
    "FIVE","SIX","SEVEN","EIGHT"
};

static bool bmpPixel(int bx, int by) {
    if (bx < 0 || bx >= 32 || by < 0 || by >= 32) return false;
    int byteIdx = by * 4 + bx / 8;
    uint8_t b   = pgm_read_byte(&bottle_bmp[byteIdx]);
    return (b >> (7 - (bx & 7))) & 1;
}

static void drawRotatedBottle(Adafruit_SSD1306 &d, float cx, float cy, float angleDeg)
{
    float rad  = angleDeg * DEG_TO_RAD;
    float cosA = cos(rad);
    float sinA = sin(rad);

    int ix0 = (int)(cx - 23), ix1 = (int)(cx + 23);
    int iy0 = (int)(cy - 23), iy1 = (int)(cy + 23);

    for (int sy = iy0; sy <= iy1; sy++) {
        if (sy < 0 || sy >= SCREEN_H) continue;
        for (int sx = ix0; sx <= ix1; sx++) {
            if (sx < 0 || sx >= SCREEN_W) continue;

            float dx = sx - cx;
            float dy = sy - cy;

            float bx = cosA * dx + sinA * dy + 15.5f;
            float by = -sinA * dx + cosA * dy + 15.5f;

            if (bmpPixel((int)(bx + 0.5f), (int)(by + 0.5f)))
                d.drawPixel(sx, sy, WHITE);
        }
    }
}

static void drawDirectionLabel(Adafruit_SSD1306 &d, int dirIndex)
{
    d.drawFastVLine(68, 0, SCREEN_H, WHITE);
    d.setTextSize(1);
    d.setTextColor(WHITE);
    const char *label = DIR_LABELS[dirIndex];
    int textW = strlen(label) * 6;
    int textX = 69 + (58 - textW) / 2;
    int textY = (SCREEN_H - 8) / 2;
    d.setCursor(textX, textY);
    d.print(label);
}

void runSpinner(Adafruit_SSD1306 &display)
{
    const float CX = 32.0f, CY = 15.5f;

    int   currentDir   = 0;
    float currentAngle = 0.0f;

    display.clearDisplay();
    drawRotatedBottle(display, CX, CY, currentAngle);
    drawDirectionLabel(display, currentDir);
    display.display();

    while (true) {
        int btn = readButton();

        if (btn == 2) return;
        if (btn == 1) {
            int newDir;
            do { newDir = random(8); } while (newDir == currentDir);

            float extraDeg = ((newDir - currentDir + 8) % 8) * 45.0f;
            float totalRot = 720.0f + extraDeg;

            const int STEPS    = 90;
            const int FRAME_MS = 17;

            for (int i = 0; i <= STEPS; i++) {
                float t    = (float)i / STEPS;
                float ease = (1.0f - cos(t * PI)) * 0.5f;
                float angle = currentAngle + ease * totalRot;

                display.clearDisplay();
                drawRotatedBottle(display, CX, CY, angle);

                display.drawFastVLine(68, 0, SCREEN_H, WHITE);
                display.setTextSize(1);
                display.setTextColor(WHITE);
                display.setCursor(80, 12);
                display.print("  :)");
                display.display();
                delay(FRAME_MS);
            }

            currentDir    = newDir;
            currentAngle += totalRot;

            display.clearDisplay();
            drawRotatedBottle(display, CX, CY, currentAngle);
            drawDirectionLabel(display, currentDir);
            display.display();
        }

        delay(20);
    }
}