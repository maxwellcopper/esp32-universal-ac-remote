#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

Preferences prefs;
WebServer server(80);
DNSServer  dns;
String cfgSSID, cfgPass, cfgAPI;
bool   apMode = false;

//proto func 
void loadWifiConfig();
void saveWifiConfig(String ssid, String pass, String api);
void sendPage(String body);
String formHTML();
void handleRoot();
void handleSave();
void startConfigPortal();
bool connectWifi();


void loadWifiConfig() {
  prefs.begin("wifi", true);
  cfgSSID = prefs.getString("ssid", "");
  cfgPass = prefs.getString("pass", "");
  cfgAPI  = prefs.getString("api",  "");
  prefs.end();
}

void saveWifiConfig(String ssid, String pass, String api) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("api",  api);
  prefs.end();
}

// ========================================
// PORTAL HTML
// ========================================
void sendPage(String body) {
  String html =
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Control AC</title><style>"
    "*{box-sizing:border-box;margin:0;padding:0}"
    "body{font:15px sans-serif;background:#f0f4f8;min-height:100vh;"
    "display:flex;justify-content:center;padding:20px}"
    ".c{background:#fff;border-radius:12px;padding:24px;width:100%;"
    "max-width:380px;box-shadow:0 2px 12px rgba(0,0,0,.12);align-self:flex-start}"
    "h2{color:#2d3748;margin-bottom:20px;font-size:1.15em;text-align:center}"
    "label{display:block;color:#4a5568;font-size:.82em;font-weight:600;margin-bottom:4px}"
    "input{width:100%;padding:9px 10px;border:1px solid #cbd5e0;border-radius:8px;"
    "font-size:.95em;margin-bottom:13px}"
    "input:focus{border-color:#4299e1;outline:none}"
    "button{width:100%;padding:11px;background:#4299e1;color:#fff;border:none;"
    "border-radius:8px;font-size:.95em;cursor:pointer;font-weight:600}"
    "button:active{background:#2b6cb0}"
    ".ok{margin-top:14px;padding:10px;border-radius:8px;background:#c6f6d5;"
    "color:#276749;text-align:center;font-size:.88em}"
    ".err{margin-top:14px;padding:10px;border-radius:8px;background:#fed7d7;"
    "color:#c53030;text-align:center;font-size:.88em}"
    "</style></head><body><div class='c'>"
    "<h2>&#127777; Control AC Setup</h2>" +
    body +
    "</div></body></html>";
  server.send(200, "text/html", html);
}

String formHTML() {
  return
    "<form method='POST' action='/save'>"
    "<label>WiFi SSID</label>"
    "<input name='ssid' placeholder='Nama WiFi' required>"
    "<label>Password</label>"
    "<input name='pass' type='password' placeholder='Password WiFi'>"
    "<label>API Key</label>"
    "<input name='api' placeholder='API Key (opsional)'>"
    "<button type='submit'>Simpan &amp; Sambung</button>"
    "</form>";
}

void handleRoot() { sendPage(formHTML()); }

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  String api  = server.arg("api");

  if (ssid.isEmpty()) {
    sendPage(formHTML() + "<div class='err'>SSID tidak boleh kosong!</div>");
    return;
  }

  saveWifiConfig(ssid, pass, api);
  sendPage("<div class='ok'>Tersimpan!<br>ESP32 menyambung ke WiFi &amp; restart...</div>");
  delay(2000);
  ESP.restart();
}

// ========================================
// WIFI CONNECT / AP MODE
// ========================================
void startConfigPortal() {
  apMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ControlAC-Setup");
  dns.start(53, "*", WiFi.softAPIP());
  server.on("/",        handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleRoot);
  server.begin();

  Serial.println("================================");
  Serial.println("  Mode: WiFi Setup Portal");
  Serial.println("  AP  : ControlAC-Setup");
  Serial.print  ("  IP  : "); Serial.println(WiFi.softAPIP());
  Serial.println("  Buka: http://192.168.4.1");
  Serial.println("================================");
}

bool connectWifi() {
  if (cfgSSID.isEmpty()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfgSSID.c_str(), cfgPass.c_str());
  Serial.print("Konek ke " + cfgSSID);
  for (uint8_t i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500); Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ IP: " + WiFi.localIP().toString());
    return true;
  }
  Serial.println("\n✗ Gagal konek WiFi → buka portal");
  return false;
}