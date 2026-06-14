#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "boot/boot.h"
#include "spinner/spinner.h"
#include "sourapple/sourapple.h"
#include "wifi/wifi.h"
#include "chathall/chathall.h"
#include "jammer/jammer.h"
#include "channelscanner/channelscanner.h"

// ═══════════════════════════════════════════════════════════════════════════
//  SHARED HARDWARE CONFIG
// ═══════════════════════════════════════════════════════════════════════════

int BTN = 0;

#define HOLD_MS    300
#define OLED_SDA   21
#define OLED_SCL   22
#define OLED_ADDR  0x3C
#define SCREEN_W   128
#define SCREEN_H   32
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);

// ═══════════════════════════════════════════════════════════════════════════
//  BUTTON LOGIC
// ═══════════════════════════════════════════════════════════════════════════

int readButton()
{
    if (digitalRead(BTN) != LOW) return 0;

    unsigned long pressTime = millis();
    while (digitalRead(BTN) == LOW) {
        if (millis() - pressTime >= HOLD_MS) {
            while (digitalRead(BTN) == LOW) delay(10);
            delay(20);
            return 2;
        }
        delay(10);
    }
    return 1;
}

// ═══════════════════════════════════════════════════════════════════════════
//  MENU
// ═══════════════════════════════════════════════════════════════════════════

struct MenuItem {
    const char *label;
    void (*run)(Adafruit_SSD1306 &);
};

static MenuItem MENU[] = {
    { "BOTTLE"        ,runSpinner        },
    { "JAMMER"        ,runJammer         },  
    { "SOUR APPLE"    ,runSourApple      },
    { "CIRCUIT TESTER",nullptr           },
    { "CHAT HALL"     ,runChathall       },
    { "WIFI"          ,runWifi           },
    { "SCANNER"       ,runChannelScanner },
};

static const int MENU_COUNT = sizeof(MENU) / sizeof(MENU[0]);
static int menuIndex = 0;

// ═══════════════════════════════════════════════════════════════════════════
//  DRAW MENU//  Layout (32px tall screen)://   
// ═══════════════════════════════════════════════════════════════════════════

static void drawMenu()
{
    display.clearDisplay();
    display.setTextWrap(false);

    // Selected item
    display.fillRect(0, 10, SCREEN_W, 10, WHITE);
    display.setTextSize(1);
    display.setTextColor(BLACK);
    int selW = strlen(MENU[menuIndex].label) * 6;
    int selX = max(0, (SCREEN_W - selW) / 2);
    display.setCursor(selX, 12);
    display.print(MENU[menuIndex].label);

    // item (above)
    int prevIdx = (menuIndex - 1 + MENU_COUNT) % MENU_COUNT;
    display.setTextSize(1);
    display.setTextColor(WHITE);
    int prevW = strlen(MENU[prevIdx].label) * 6;
    int prevX = max(0, (SCREEN_W - prevW) / 2);
    display.setCursor(prevX, 1);
    display.print(MENU[prevIdx].label);

    // item (below)
    int nextIdx = (menuIndex + 1) % MENU_COUNT;
    int nextW = strlen(MENU[nextIdx].label) * 6;
    int nextX = max(0, (SCREEN_W - nextW) / 2);
    display.setCursor(nextX, 24);
    display.print(MENU[nextIdx].label);

    display.display();
}

// ═══════════════════════════════════════════════════════════════════════════
//  COMING SOON
// ═══════════════════════════════════════════════════════════════════════════

static void showNotImplemented()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(10, 5);
    display.print("NOT IMPLEMENTED");
    display.setCursor(22, 18);
    display.print("COMING SOON...");
    display.display();
    delay(1000);
}

// ═══════════════════════════════════════════════════════════════════════════
//  SETUP / LOOP
// ═══════════════════════════════════════════════════════════════════════════

void setup()
{
    Serial.begin(115200);
    pinMode(BTN, INPUT_PULLUP);

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 init failed");
        while (true) delay(100);
    }
    display.setTextWrap(false);
    display.dim(false);

    showBootScreen(display);
    drawMenu();
}

void loop()
{
    int btn = readButton();

    if (btn == 1) {
        menuIndex = (menuIndex + 1) % MENU_COUNT;
        drawMenu();
    }
    else if (btn == 2) {
        if (MENU[menuIndex].run != nullptr) {
            MENU[menuIndex].run(display);
        } else {
            showNotImplemented();
        }
        drawMenu();
    }

    delay(20);
}