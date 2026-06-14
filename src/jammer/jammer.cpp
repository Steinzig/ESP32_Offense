#include "jammer.h"
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

extern int readButton();

#define CE_PIN  4
#define CSN_PIN 5

#define SCREEN_W 128
#define SCREEN_H  32

static RF24 radio(CE_PIN, CSN_PIN);

// ── Mode definitions ─────────────────────────────────────────────────────────
struct JamMode {
    const char *name;
    uint8_t     chStart;
    uint8_t     chEnd;
};

static const JamMode MODES[] = {
    { "FULL SPECTRUM", 0,   125 },
    { "WIFI",          1,   13  },
    { "BLUETOOTH",     0,   78  },
};
static const int MODE_COUNT = 3;

// ── OLED draw ─────────────────────────────────────────────────────────────────
static void drawJamUI(Adafruit_SSD1306 &d,
                      int modeIdx, bool jamming, uint8_t channel)
{
    d.clearDisplay();
    d.setTextWrap(false);

    // Top bar — mode name, inverted if jamming
    if (jamming) {
        d.fillRect(0, 0, SCREEN_W, 10, WHITE);
        d.setTextColor(BLACK);
    } else {
        d.setTextColor(WHITE);
    }
    d.setTextSize(1);
    const char *modeName = MODES[modeIdx].name;
    int modeW = strlen(modeName) * 6;
    int modeX = max(0, (SCREEN_W - modeW) / 2);
    d.setCursor(modeX, 1);
    d.print(modeName);

    // Middle — status
    d.setTextColor(WHITE);
    d.setTextSize(1);
    d.setCursor(0, 13);
    d.print(jamming ? "JAMMING" : "HOLD=JAM");
    if (jamming) {
        d.setCursor(70, 13);
        d.print("CH:");
        d.print(channel);
    }

    // Bottom — controls hint
    d.setCursor(0, 24);
    d.print("TAP=MODE  DBL=BACK");

    d.display();
}

// ── Mode select screen ────────────────────────────────────────────────────────
static void drawModeSelect(Adafruit_SSD1306 &d, int modeIdx)
{
    d.clearDisplay();
    d.setTextWrap(false);

    // Selected mode — inverted centre
    d.fillRect(0, 10, SCREEN_W, 10, WHITE);
    d.setTextColor(BLACK);
    d.setTextSize(1);
    const char *cur = MODES[modeIdx].name;
    int curW = strlen(cur) * 6;
    int curX = max(0, (SCREEN_W - curW) / 2);
    d.setCursor(curX, 12);
    d.print(cur);

    // Previous mode
    int prevIdx = (modeIdx - 1 + MODE_COUNT) % MODE_COUNT;
    d.setTextColor(WHITE);
    const char *prev = MODES[prevIdx].name;
    int prevW = strlen(prev) * 6;
    int prevX = max(0, (SCREEN_W - prevW) / 2);
    d.setCursor(prevX, 1);
    d.print(prev);

    // Next mode
    int nextIdx = (modeIdx + 1) % MODE_COUNT;
    const char *next = MODES[nextIdx].name;
    int nextW = strlen(next) * 6;
    int nextX = max(0, (SCREEN_W - nextW) / 2);
    d.setCursor(nextX, 24);
    d.print(next);

    d.display();
}

// ── Jam tick — call repeatedly while jamming ──────────────────────────────────
static uint8_t currentChannel = 0;

static void jamTick(int modeIdx)
{
    uint8_t start = MODES[modeIdx].chStart;
    uint8_t end   = MODES[modeIdx].chEnd;

    radio.setChannel(currentChannel);
    radio.startConstCarrier(RF24_PA_MAX, currentChannel);
    delayMicroseconds(200);
    radio.stopConstCarrier();

    currentChannel++;
    if (currentChannel > end) currentChannel = start;
}

// ── Entry point ───────────────────────────────────────────────────────────────
void runJammer(Adafruit_SSD1306 &display)
{
    // Init NRF24
    if (!radio.begin()) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(10, 5);
        display.print("NRF24 NOT FOUND");
        display.setCursor(10, 18);
        display.print("CHECK WIRING");
        display.display();
        // wait for double tap to exit
        while (true) {
            int b = readButton();
            if (b == 3 || b == 2 || b == 1) return;
            delay(20);
        }
    }

    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_2MBPS);
    radio.setAutoAck(false);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.startListening();

    int  modeIdx  = 0;
    bool jamming  = false;
    bool selecting = true;   // start in mode select
    currentChannel = MODES[modeIdx].chStart;

    drawModeSelect(display, modeIdx);

    unsigned long lastDraw = 0;

    while (true) {
        int btn = readButton();

        // ── Double tap = BACK (exit jammer) ───────────────────────────────
        if (btn == 3) {
            if (jamming) {
                radio.stopConstCarrier();
                radio.powerDown();
            }
            return;
        }

        // ── In mode select ────────────────────────────────────────────────
        if (selecting) {
            if (btn == 1) {
                // single tap = next mode
                modeIdx = (modeIdx + 1) % MODE_COUNT;
                currentChannel = MODES[modeIdx].chStart;
                drawModeSelect(display, modeIdx);
            }
            else if (btn == 2) {
                // hold = confirm mode, go to jam screen
                selecting = false;
                jamming   = false;
                drawJamUI(display, modeIdx, false, currentChannel);
            }
            delay(20);
            continue;
        }

        // ── In jam screen ─────────────────────────────────────────────────
        if (btn == 1) {
            // single tap = back to mode select
            if (jamming) {
                radio.stopConstCarrier();
                jamming = false;
            }
            selecting = true;
            currentChannel = MODES[modeIdx].chStart;
            drawModeSelect(display, modeIdx);
        }
        else if (btn == 2) {
            // hold = toggle jam on/off
            jamming = !jamming;
            if (!jamming) {
                radio.stopConstCarrier();
            }
            drawJamUI(display, modeIdx, jamming, currentChannel);
        }

        // ── Jam tick + OLED refresh ───────────────────────────────────────
        if (jamming) {
            jamTick(modeIdx);

            // update display every 100ms so channel number scrolls
            if (millis() - lastDraw > 100) {
                drawJamUI(display, modeIdx, true, currentChannel);
                lastDraw = millis();
            }
        }

        delay(1);
    }
}