#include "channelscanner.h"
#include <Arduino.h>
#include <WiFi.h>

extern int readButton();

#define SCREEN_W   128
#define SCREEN_H    32
#define CH_COUNT    13
#define SCAN_INTERVAL 1000   // ms between scans

// Each channel occupies ~9px width across 128px
// Channels 1-13 spread across full width
// Channel x centre = map(x, 1, 13, 6, 122)

static int  chStrength[CH_COUNT + 1];  // index 1-13, RSSI mapped to height
static char chSSID[CH_COUNT + 1][33];  // strongest SSID per channel

// Map channel number to X pixel centre
static int chToX(int ch)
{
    return map(ch, 1, 13, 6, 122);
}

// Map RSSI (-100 to -30) to bar height (1 to 28)
static int rssiToHeight(int rssi)
{
    if (rssi == 0) return 0;
    return map(constrain(rssi, -100, -30), -100, -30, 2, 22);
}

// Draw a gaussian-ish arch for one network
// centred on chX, height h, width ~20px
static void drawArch(Adafruit_SSD1306 &d, int centreX, int h)
{
    if (h <= 0) return;
    int baseY = SCREEN_H - 1;

    // Draw arch as a series of vertical lines following a bell curve
    // arch spreads ±10px from centre
    for (int dx = -10; dx <= 10; dx++) {
        int x = centreX + dx;
        if (x < 0 || x >= SCREEN_W) continue;

        // Bell curve: y = h * e^(-dx^2 / 20)
        float t = (float)(dx * dx) / 20.0f;
        int archH = (int)(h * exp(-t));
        if (archH < 1) continue;

        int y = baseY - archH;
        d.drawFastVLine(x, y, archH, WHITE);
    }
}

static void drawUI(Adafruit_SSD1306 &d)
{
    d.clearDisplay();

    // Baseline
    d.drawFastHLine(0, SCREEN_H - 1, SCREEN_W, WHITE);

    // Arches
    for (int ch = 1; ch <= CH_COUNT; ch++) {
        if (chStrength[ch] == 0) continue;
        int cx = chToX(ch);
        int h  = rssiToHeight(chStrength[ch]);
        drawArch(d, cx, h);
    }

    // Channel labels — no background, white text
    int labelChs[] = {1, 3, 5, 7, 9, 11, 13};
    for (int i = 0; i < 7; i++) {
        int ch  = labelChs[i];
        int cx  = chToX(ch);
        int lblW = (ch >= 10 ? 12 : 6);
        int lblX = cx - lblW / 2;
        d.setTextSize(1);
        d.setTextColor(WHITE);
        d.setCursor(lblX, 0);
        d.print(ch);
    }

    d.display();
}

static void doScan()
{
    // Clear previous
    for (int i = 1; i <= CH_COUNT; i++) {
        chStrength[i] = 0;
        chSSID[i][0]  = '\0';
    }

    int n = WiFi.scanNetworks(false, true);  // blocking, include hidden
    if (n <= 0) return;

    for (int i = 0; i < n; i++) {
        int ch = WiFi.channel(i);
        if (ch < 1 || ch > CH_COUNT) continue;

        int rssi = WiFi.RSSI(i);

        // Keep strongest signal per channel
        if (chStrength[ch] == 0 || rssi > chStrength[ch]) {
            chStrength[ch] = rssi;
            strncpy(chSSID[ch], WiFi.SSID(i).c_str(), 32);
            chSSID[ch][32] = '\0';
        }
    }

    WiFi.scanDelete();
}

void runChannelScanner(Adafruit_SSD1306 &display)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Clear arrays
    for (int i = 1; i <= CH_COUNT; i++) {
        chStrength[i] = 0;
        chSSID[i][0]  = '\0';
    }

    // Show scanning message
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(30, 12);
    display.print("SCANNING...");
    display.display();

    doScan();
    drawUI(display);

    unsigned long lastScan = millis();

    while (true) {
        int btn = readButton();

        // single tap or double tap = exit
        if (btn == 1) {
            WiFi.scanDelete();
            return;
        }

        // Auto rescan every SCAN_INTERVAL ms
        if (millis() - lastScan >= SCAN_INTERVAL) {
            doScan();
            drawUI(display);
            lastScan = millis();
        }

        delay(20);
    }
}