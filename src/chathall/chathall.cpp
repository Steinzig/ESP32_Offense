#include "chathall.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "esp_wifi.h"

extern int readButton();

#define SCREEN_W     128
#define SCREEN_H      32
#define DNS_PORT      53
#define MAX_CLIENTS   10
#define MAX_MESSAGES  50

static char *SSID = "Hall";
const char  *PSW  = "skateboard";

static WebServer webServer(80);
static DNSServer dnsServer;

struct Message {
    char user[16];
    char text[128];
};

static Message messages[MAX_MESSAGES];
static int     msgCount  = 0;
static int     userCount = 0;

// ═══════════════════════════════════════════════════════════════════════════
//  HTML
// ═══════════════════════════════════════════════════════════════════════════

static const char PAGE_HTML[] PROGMEM = R"(
<!DOCTYPE html><html><head>
<meta name='viewport' content='width=device-width,initial-scale=1,viewport-fit=cover'>
<title>Hall Chat</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&display=swap');
*{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#000;
  --bg2:#0a0a0a;
  --bg3:#111;
  --accent:#4fc3f7;
  --accent2:#0288d1;
  --text:#e0f7fa;
  --dim:#546e7a;
  --border:#1a2a3a;
}
body{
  background:var(--bg);
  color:var(--text);
  font-family:'Share Tech Mono',monospace;
  display:flex;
  flex-direction:column;
  height:100dvh;
  overflow:hidden;
  position:fixed;
  width:100%;
}
#header{
  background:var(--bg2);
  border-bottom:1px solid var(--accent2);
  padding:10px 16px 4px;
  font-size:16px;
  letter-spacing:3px;
  color:var(--accent);
  text-transform:uppercase;
  flex-shrink:0;
}
#sub{
  background:var(--bg2);
  border-bottom:1px solid var(--border);
  padding:2px 16px 8px;
  font-size:11px;
  color:var(--dim);
  letter-spacing:1px;
  flex-shrink:0;
}
#msgs{
  flex:1;
  overflow-y:auto;
  padding:12px;
  padding-bottom:4px;
  display:flex;
  flex-direction:column;
  gap:6px;
  scrollbar-width:thin;
  scrollbar-color:var(--accent2) var(--bg);
  -webkit-overflow-scrolling:touch;
}
#msgs::-webkit-scrollbar{width:4px}
#msgs::-webkit-scrollbar-track{background:var(--bg)}
#msgs::-webkit-scrollbar-thumb{background:var(--accent2);border-radius:2px}
.bubble{
  max-width:80%;
  padding:6px 10px;
  border-radius:2px;
  font-size:13px;
  line-height:1.5;
  word-break:break-word;
  border-left:2px solid transparent;
}
.bubble .name{
  font-size:10px;
  letter-spacing:2px;
  margin-bottom:2px;
  color:var(--accent);
  text-transform:uppercase;
}
.bubble .time{
  font-size:10px;
  color:var(--dim);
  text-align:right;
  margin-top:2px;
}
.mine{
  background:#001a2e;
  border-left:2px solid var(--accent);
  align-self:flex-end;
}
.other{
  background:var(--bg3);
  border-left:2px solid var(--accent2);
  align-self:flex-start;
}
.system{
  align-self:center;
  background:transparent;
  border-left:none;
  color:var(--dim);
  font-size:11px;
  letter-spacing:1px;
  text-transform:uppercase;
}
#bar{
  display:flex;
  padding:8px;
  padding-bottom:calc(8px + env(safe-area-inset-bottom));
  background:var(--bg2);
  border-top:1px solid var(--border);
  gap:8px;
  align-items:center;
  position:sticky;
  bottom:0;
  width:100%;
  flex-shrink:0;
}
#msg{
  flex:1;
  background:var(--bg3);
  color:var(--text);
  border:1px solid var(--border);
  border-radius:2px;
  padding:8px 12px;
  font-size:16px;
  font-family:'Share Tech Mono',monospace;
  outline:none;
  letter-spacing:1px;
  -webkit-appearance:none;
  min-width:0;
}
#msg:focus{border-color:var(--accent2)}
#sendbtn{
  background:var(--accent2);
  color:#000;
  border:none;
  border-radius:2px;
  width:42px;
  height:42px;
  font-size:18px;
  cursor:pointer;
  font-weight:bold;
  transition:background .15s;
  flex-shrink:0;
}
#sendbtn:active{background:var(--accent)}
#join{
  display:flex;
  flex-direction:column;
  align-items:center;
  justify-content:center;
  height:100dvh;
  gap:14px;
  padding:32px;
  position:fixed;
  width:100%;
}
#join h2{
  color:var(--accent);
  letter-spacing:6px;
  font-size:20px;
  margin-bottom:8px;
  text-transform:uppercase;
}
#join p.sub{
  color:var(--dim);
  font-size:11px;
  letter-spacing:2px;
  margin-top:-8px;
}
#join input{
  width:100%;
  background:var(--bg3);
  color:var(--text);
  border:1px solid var(--border);
  border-radius:2px;
  padding:12px 16px;
  font-size:16px;
  font-family:'Share Tech Mono',monospace;
  outline:none;
  letter-spacing:2px;
  -webkit-appearance:none;
}
#join input:focus{border-color:var(--accent2)}
#join button{
  width:100%;
  background:var(--accent2);
  color:#000;
  border:none;
  border-radius:2px;
  padding:12px;
  font-size:14px;
  font-family:'Share Tech Mono',monospace;
  font-weight:bold;
  cursor:pointer;
  letter-spacing:4px;
  text-transform:uppercase;
  transition:background .15s;
}
#join button:active{background:var(--accent)}
#err{color:#ef5350;font-size:11px;letter-spacing:1px}
body::after{
  content:'';
  position:fixed;
  top:0;left:0;
  width:100%;height:100%;
  background:repeating-linear-gradient(
    0deg,transparent,transparent 2px,
    rgba(0,0,0,0.08) 2px,rgba(0,0,0,0.08) 4px
  );
  pointer-events:none;
  z-index:999;
}
</style>
</head><body>

<div id='join'>
  <h2>HALL CHAT</h2>
  <p class='sub'>ESP32 CAVE // LOCAL NETWORK</p>
  <input id='uname' placeholder='ENTER CALLSIGN' maxlength='15'
         autocomplete='off' autocorrect='off' autocapitalize='off'
         onkeydown='if(event.key==="Enter")join()'>
  <button type='button' onclick='join()'>[ CONNECT ]</button>
  <p id='err'></p>
</div>

<div id='chat' style='display:none;flex-direction:column;height:100dvh;position:fixed;width:100%;'>
  <div id='header'>&#9608; HALL CHAT</div>
  <div id='sub'>-- CONNECTING --</div>
  <div id='msgs'></div>
  <div id='bar'>
    <input id='msg' placeholder='> TYPE MESSAGE...' maxlength='127'
           autocomplete='off' autocorrect='off' autocapitalize='off'
           onkeydown='if(event.key==="Enter")sendMsg()'>
    <button id='sendbtn' type='button' onclick='sendMsg()'>&#10148;</button>
  </div>
</div>

<script>
let me='', last=-1;

function join(){
  let n=document.getElementById('uname').value.trim();
  if(!n)return;
  fetch('/join?name='+encodeURIComponent(n))
    .then(r=>r.json()).then(d=>{
      if(d.ok){
        me=n;
        document.getElementById('join').style.display='none';
        document.getElementById('chat').style.display='flex';
        poll();
      } else {
        document.getElementById('err').innerText='// '+d.err;
      }
    }).catch(()=>{
      document.getElementById('err').innerText='// CONNECTION FAILED';
    });
}

function sendMsg(){
  let t=document.getElementById('msg').value.trim();
  if(!t)return;
  document.getElementById('msg').value='';
  fetch('/send?name='+encodeURIComponent(me)+'&msg='+encodeURIComponent(t))
    .catch(()=>{});
}

function poll(){
  fetch('/msgs?since='+last)
    .then(r=>r.json()).then(d=>{
      let online=d.users;
      document.getElementById('sub').innerText=
        '-- '+online+' UNIT'+(online===1?'':'S')+' ONLINE // 192.168.4.1 --';
      d.msgs.forEach(m=>{
        last=m.id;
        let div=document.createElement('div');
        let isSystem=m.user==='SYSTEM';
        div.className='bubble '+(isSystem?'system':m.user===me?'mine':'other');
        if(isSystem){
          div.innerHTML='// '+m.text;
        } else {
          div.innerHTML=
            (m.user!==me?'<div class="name">> '+m.user+'</div>':'')+
            '<div>'+m.text+'</div>'+
            '<div class="time">'+m.time+'</div>';
        }
        document.getElementById('msgs').appendChild(div);
      });
      let ms=document.getElementById('msgs');
      ms.scrollTop=ms.scrollHeight;
    }).catch(()=>{}).finally(()=>setTimeout(poll,1000));
}

// fix samsung keyboard pushing layout
window.addEventListener('resize',()=>{
  let ms=document.getElementById('msgs');
  if(ms) ms.scrollTop=ms.scrollHeight;
});
</script>
</body></html>
)";

// ═══════════════════════════════════════════════════════════════════════════
//  OLED UI
// ═══════════════════════════════════════════════════════════════════════════

static void drawUI(Adafruit_SSD1306 &d, bool run, int users, int msgs)
{
    d.clearDisplay();
    d.setTextWrap(false);

    d.fillRect(0, 0, SCREEN_W, 10, WHITE);
    d.setTextColor(BLACK);
    d.setTextSize(1);
    d.setCursor(2, 1);
    d.print("CHAT HALL");
    d.setCursor(86, 1);
    d.print(run ? "RUNNING" : "STOPPED");

    d.setTextColor(WHITE);
    d.setCursor(0, 13);
    d.print("USERS: ");
    d.print(users);
    d.print("/");
    d.print(MAX_CLIENTS);

    d.setCursor(0, 23);
    d.print("MSGS:  ");
    d.print(msgs);

    d.display();
}

// ═══════════════════════════════════════════════════════════════════════════
//  SERVER ROUTES
// ═══════════════════════════════════════════════════════════════════════════

static void startServer()
{
    webServer.onNotFound([]() {
        webServer.send(200, "text/html", FPSTR(PAGE_HTML));
    });

    webServer.on("/join", []() {
        if (!webServer.hasArg("name")) {
            webServer.send(400); return;
        }
        String name = webServer.arg("name");
        name.trim();

        int cur = WiFi.softAPgetStationNum();
        if (cur >= MAX_CLIENTS) {
            webServer.send(200, "application/json",
                "{\"ok\":false,\"err\":\"Room full (10/10)\"}");
            return;
        }

        if (msgCount < MAX_MESSAGES) {
            snprintf(messages[msgCount].user, 16, "SYSTEM");
            snprintf(messages[msgCount].text, 128, "%s joined", name.c_str());
            msgCount++;
        }
        userCount = cur + 1;

        Serial.printf("JOIN | %s\n", name.c_str());
        webServer.send(200, "application/json", "{\"ok\":true}");
    });

    webServer.on("/send", []() {
        if (!webServer.hasArg("name") || !webServer.hasArg("msg")) {
            webServer.send(400); return;
        }
        String name = webServer.arg("name");
        String text = webServer.arg("msg");

        if (msgCount < MAX_MESSAGES) {
            snprintf(messages[msgCount].user, 16, "%s", name.c_str());
            snprintf(messages[msgCount].text, 128, "%s", text.c_str());
            msgCount++;
        } else {
            memmove(&messages[0], &messages[1],
                sizeof(Message) * (MAX_MESSAGES - 1));
            snprintf(messages[MAX_MESSAGES-1].user, 16, "%s", name.c_str());
            snprintf(messages[MAX_MESSAGES-1].text, 128, "%s", text.c_str());
        }

        Serial.printf("MSG | %s: %s\n", name.c_str(), text.c_str());
        webServer.send(200, "application/json", "{\"ok\":true}");
    });

    webServer.on("/msgs", []() {
        int since = -1;
        if (webServer.hasArg("since"))
            since = webServer.arg("since").toInt();

        String json = "{\"users\":";
        json += WiFi.softAPgetStationNum();
        json += ",\"msgs\":[";

        bool first = true;
        for (int i = 0; i < msgCount; i++) {
            if (i <= since) continue;
            if (!first) json += ",";
            first = false;

            unsigned long s = millis() / 1000;
            char t[8];
            snprintf(t, 8, "%02lu:%02lu", (s % 3600) / 60, s % 60);

            json += "{\"id\":";
            json += i;
            json += ",\"user\":\"";
            json += messages[i].user;
            json += "\",\"text\":\"";
            json += messages[i].text;
            json += "\",\"time\":\"";
            json += t;
            json += "\"}";
        }
        json += "]}";
        webServer.send(200, "application/json", json);
    });

    webServer.begin();
}

// ═══════════════════════════════════════════════════════════════════════════
//  ENTRY POINT
// ═══════════════════════════════════════════════════════════════════════════

void runChathall(Adafruit_SSD1306 &display)
{
    msgCount  = 0;
    userCount = 0;
    bool running = true;

    IPAddress ip(192, 168, 4, 1);
    WiFi.softAP(SSID, PSW, 1, 0, MAX_CLIENTS);
    delay(500);
    WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));

    dnsServer.start(DNS_PORT, "*", ip);
    startServer();

    drawUI(display, running, 0, 0);

    while (true) {
        int btn = readButton();

        // BACK — exit
        if (btn == 2) {
            webServer.stop();
            dnsServer.stop();
            WiFi.softAPdisconnect(true);
            return;
        }

        // SELECT — toggle on/off
        if (btn == 1) {
            running = !running;
            if (!running) {
                webServer.stop();
                dnsServer.stop();
                WiFi.softAPdisconnect(true);
            } else {
                WiFi.softAP(SSID, PSW, 1, 0, MAX_CLIENTS);
                delay(500);
                WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
                dnsServer.start(DNS_PORT, "*", ip);
                startServer();
            }
            drawUI(display, running, WiFi.softAPgetStationNum(), msgCount);
        }

        if (running) {
            dnsServer.processNextRequest();
            webServer.handleClient();

            int cur = WiFi.softAPgetStationNum();
            if (cur != userCount) {
                userCount = cur;
                drawUI(display, running, userCount, msgCount);
            }
        }

        delay(10);
    }
}