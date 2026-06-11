#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

struct WifiManagerHandler {
  Preferences prefs;
  WebServer server;
  DNSServer dns;

  String ssid;
  String pass;
  String serverBaseUrl;

  bool apMode;

  WifiManagerHandler();

  void begin();
  void loop();

  void loadConfig();
  void saveConfig(const String& newSsid, const String& newPass, const String& newApiKey);

  bool connectWifi();
  void startConfigPortal();

  void sendPage(const String& body);
  String formHTML();

  void handleRoot();
  void handleSave();

  void resetWifi();
  void resetApi();

  bool isApMode() const;
  bool isConnected() const;
  String getIpString() const;
  String getApiKey() const;
  String getServerBaseUrl();
};

extern WifiManagerHandler wifiManager;

#endif