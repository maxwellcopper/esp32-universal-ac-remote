#include "wifiManager.h"

WifiManagerHandler wifiManager;

// Wrapper untuk callback WebServer
// Karena server.on() lebih aman pakai function biasa.
static void handleRootCallback() {
  wifiManager.handleRoot();
}

static void handleSaveCallback() {
  wifiManager.handleSave();
}

WifiManagerHandler::WifiManagerHandler()
  : server(80),
    apMode(false)
{
}

/*
open wifi config condition : 
- no WIFI SSID
- no server base url
- fail to connect wifi

*/
void WifiManagerHandler::begin() {
  loadConfig();

  if (ssid.isEmpty() || serverBaseUrl.isEmpty()) {
    startConfigPortal();
    return;
  }

  if (!connectWifi()) {
    startConfigPortal();
  }
}

void WifiManagerHandler::loop() {
    if (apMode) {
        dns.processNextRequest();
        server.handleClient();
      }
}

void WifiManagerHandler::loadConfig() {
  prefs.begin("wifi", true);
  ssid   = prefs.getString("ssid", "");
  pass   = prefs.getString("pass", "");
  serverBaseUrl = prefs.getString("base_url", "");
  prefs.end();
}

void WifiManagerHandler::saveConfig(const String& newSsid,
                                    const String& newPass,
                                    const String& newServerBaseUrl) {

  String fixedBaseUrl = newServerBaseUrl;
  fixedBaseUrl.trim();

  if (fixedBaseUrl.endsWith("/")) {
    fixedBaseUrl.remove(fixedBaseUrl.length() - 1);
  }                                    

  prefs.begin("wifi", false);
  prefs.putString("ssid", newSsid);
  prefs.putString("pass", newPass);
  prefs.putString("base_url", newServerBaseUrl);
  prefs.end();

  ssid          = newSsid;
  pass          = newPass;
  serverBaseUrl = fixedBaseUrl;
}

bool WifiManagerHandler::connectWifi() {
  if (ssid.isEmpty()) {
    return false;
  }

  apMode = false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.print("Konek ke ");
  Serial.print(ssid);

  for (uint8_t i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("✓ IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println();
  Serial.println("✗ Gagal konek WiFi → buka portal");
  return false;
}

void WifiManagerHandler::startConfigPortal() {
  apMode = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAP("ControlAC-Setup");

  dns.start(53, "*", WiFi.softAPIP());

  server.on("/", handleRootCallback);
  server.on("/save", HTTP_POST, handleSaveCallback);
  server.onNotFound(handleRootCallback);

  server.begin();

  Serial.println("================================");
  Serial.println("  Mode: WiFi Setup Portal");
  Serial.println("  AP  : ControlAC-Setup");
  Serial.print  ("  IP  : ");
  Serial.println(WiFi.softAPIP());
  Serial.println("  Buka: http://192.168.4.1");
  Serial.println("================================");
}

void WifiManagerHandler::sendPage(const String& body) {
  String html;

  html += "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Control AC</title><style>";
  html += "*{box-sizing:border-box;margin:0;padding:0}";
  html += "body{font:15px sans-serif;background:#f0f4f8;min-height:100vh;";
  html += "display:flex;justify-content:center;padding:20px}";
  html += ".c{background:#fff;border-radius:12px;padding:24px;width:100%;";
  html += "max-width:380px;box-shadow:0 2px 12px rgba(0,0,0,.12);align-self:flex-start}";
  html += "h2{color:#2d3748;margin-bottom:20px;font-size:1.15em;text-align:center}";
  html += "label{display:block;color:#4a5568;font-size:.82em;font-weight:600;margin-bottom:4px}";
  html += "input{width:100%;padding:9px 10px;border:1px solid #cbd5e0;border-radius:8px;";
  html += "font-size:.95em;margin-bottom:13px}";
  html += "input:focus{border-color:#4299e1;outline:none}";
  html += "button{width:100%;padding:11px;background:#4299e1;color:#fff;border:none;";
  html += "border-radius:8px;font-size:.95em;cursor:pointer;font-weight:600}";
  html += "button:active{background:#2b6cb0}";
  html += ".ok{margin-top:14px;padding:10px;border-radius:8px;background:#c6f6d5;";
  html += "color:#276749;text-align:center;font-size:.88em}";
  html += ".err{margin-top:14px;padding:10px;border-radius:8px;background:#fed7d7;";
  html += "color:#c53030;text-align:center;font-size:.88em}";
  html += "</style></head><body><div class='c'>";
  html += "<h2>&#127777; Control AC Setup</h2>";
  html += body;
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

String WifiManagerHandler::formHTML() {
  String html;

  html += "<form method='POST' action='/save'>";

  html += "<label>WiFi SSID</label>";
  html += "<input name='ssid' placeholder='Nama WiFi' required>";

  html += "<label>Password</label>";
  html += "<input name='pass' type='password' placeholder='Password WiFi'>";

  html += "<label>Server Base URL</label>";
  html += "<input name='base_url' placeholder='contoh: http://192.168.1.50' required>";

  html += "<button type='submit'>Simpan &amp; Sambung</button>";
  html += "</form>";

  return html;
}

void WifiManagerHandler::handleRoot() {
  sendPage(formHTML());
}

void WifiManagerHandler::handleSave() {
  String newSsid          = server.arg("ssid");
  String newPass          = server.arg("pass");
  String newServerBaseUrl = server.arg("base_url");

  newSsid.trim();
  newPass.trim();
  newServerBaseUrl.trim();

  if (newSsid.isEmpty()) {
    sendPage(formHTML() + "<div class='err'>SSID tidak boleh kosong!</div>");
    return;
  }

  if (newServerBaseUrl.isEmpty()) {
    sendPage(formHTML() + "<div class='err'>Server Base URL tidak boleh kosong!</div>");
    return;
  }

  saveConfig(newSsid, newPass, newServerBaseUrl);

  sendPage("<div class='ok'>Tersimpan!<br>ESP32 menyambung ke WiFi &amp; restart...</div>");

  delay(2000);
  ESP.restart();
}

void WifiManagerHandler::resetWifi() {
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();

  ssid   = "";
  pass   = "";
  serverBaseUrl = "";

  Serial.println("✓ WiFi dihapus! Restart ke portal...");
  delay(1000);
  ESP.restart();
}

bool WifiManagerHandler::isApMode() const {
  return apMode;
}

bool WifiManagerHandler::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WifiManagerHandler::getIpString() const {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }

  if (apMode) {
    return "Portal";
  }

  return "Offline";
}

String WifiManagerHandler::getServerBaseUrl() {
  return serverBaseUrl;
}