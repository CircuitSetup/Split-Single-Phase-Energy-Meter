/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
   All adaptation GNU General Public License as below.

   -------------------------------------------------------------------

   This file is part of OpenEnergyMonitor.org project.
   EmonESP is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   EmonESP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with EmonESP; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "emonesp.h"
#include "wifi.h"
#include "config.h"

#ifdef ESP32
#include <esp_wifi.h>              // Connect to Wifi
#include <WiFi.h>
#include <ESPmDNS.h>              // Resolve URL for update server etc.
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>              // Resolve URL for update server etc.
#endif

#include <DNSServer.h>                // Required for captive portal

DNSServer dnsServer;                  // Create class DNS server, captive portal re-direct
const byte DNS_PORT = 53;

// Access Point SSID, password & IP address. SSID will be softAP_ssid + chipID to make SSID unique
const char *softAP_ssid = "emonESP";
const char* softAP_password = "";
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// hostname for mDNS. Should work at least on windows. Try http://emonesp.local
const char *esp_hostname = "emonesp";

// Wifi Network Strings
String connected_network = "";
String status_string = "";
String ipaddress = "";

unsigned long Timer;
String st, rssi;

#ifdef WIFI_LED
#ifndef WIFI_LED_ON_STATE
#define WIFI_LED_ON_STATE LOW
#endif

#ifndef WIFI_LED_AP_TIME
#define WIFI_LED_AP_TIME 1000
#endif

#ifndef WIFI_LED_STA_CONNECTING_TIME
#define WIFI_LED_STA_CONNECTING_TIME 500
#endif

int wifiLedState = !WIFI_LED_ON_STATE;
unsigned long wifiLedTimeOut = millis();
#endif

// -------------------------------------------------------------------
int wifi_mode = WIFI_MODE_CLIENT;


// -------------------------------------------------------------------
// Start Access Point
// Access point is used for wifi network selection
// -------------------------------------------------------------------
void
startAP() {
  DEBUG.print("Starting AP");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  DEBUG.print("Scan: ");
  int n = WiFi.scanNetworks();
  DEBUG.print(n);
  DEBUG.println(" networks found");
  st = "";
  rssi = "";
  for (int i = 0; i < n; ++i) {
    st += "\"" + WiFi.SSID(i) + "\"";
    rssi += "\"" + String(WiFi.RSSI(i)) + "\"";
    if (i < n - 1)
      st += ",";
    if (i < n - 1)
      rssi += ",";
  }
  delay(100);

  WiFi.softAPConfig(apIP, apIP, netMsk);

  // Create Unique SSID e.g "emonESP_XXXXXX"
#ifdef ESP32
  String softAP_ssid_ID =
    String(softAP_ssid) + "_" + String((uint32_t)ESP.getEfuseMac());
#else
  String softAP_ssid_ID =
    String(softAP_ssid) + "_" + String(ESP.getChipId());
#endif

  // Pick a random channel out of 1, 6 or 11
  int channel = (random(3) * 5) + 1;
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password, channel);

  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  DEBUG.print("AP IP Address: ");
  DEBUG.println(tmpStr);
  ipaddress = tmpStr;
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
void startClient() {
  DEBUG.print("Connecting to SSID: ");
  DEBUG.println(esid.c_str());
  // DEBUG.print(" epass:");
  // DEBUG.println(epass.c_str());

  WiFi.begin(esid.c_str(), epass.c_str());
#ifndef ESP32
  WiFi.hostname(esp_hostname);
#endif // !ESP32
  WiFi.enableSTA(true);

  delay(50);

  int t = 0;
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
#ifdef WIFI_LED
    wifiLedState = !wifiLedState;
    digitalWrite(WIFI_LED, wifiLedState);
#endif

    delay(500);
    t++;
    // push and hold boot button after power on to skip stright to AP mode
    if (t >= 20
#if !defined(WIFI_LED) || 0 != WIFI_LED
        || digitalRead(0) == LOW
#endif
       ) {
      DEBUG.println(" ");
      DEBUG.println("Try Again...");
      delay(2000);
      WiFi.disconnect();
      WiFi.begin(esid.c_str(), epass.c_str());
      t = 0;
      attempt++;
      if (attempt >= 5 || digitalRead(0) == LOW) {
        startAP();
        // AP mode with SSID in EEPROM, connection will retry in 5 minutes
        wifi_mode = WIFI_MODE_AP_STA_RETRY;
        break;
      }
    }
  }

  if (wifi_mode == WIFI_MODE_CLIENT || wifi_mode == WIFI_MODE_AP_AND_STA) {
#ifdef WIFI_LED
    wifiLedState = WIFI_LED_ON_STATE;
    digitalWrite(WIFI_LED, wifiLedState);
#endif

    IPAddress myAddress = WiFi.localIP();
    char tmpStr[40];
    sprintf(tmpStr, "%d.%d.%d.%d", myAddress[0], myAddress[1], myAddress[2],
            myAddress[3]);
    DEBUG.print("Connected, IP: ");
    DEBUG.println(tmpStr);
    // Copy the connected network and ipaddress to global strings for use in status request
    connected_network = esid;
    ipaddress = tmpStr;
  }
}

void wifi_setup() {
#ifdef WIFI_LED
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, wifiLedState);
#endif

  WiFi.disconnect();
  // 1) If no network configured start up access point
  if (esid == 0 || esid == "") {
    startAP();
    wifi_mode = WIFI_MODE_AP_ONLY; // AP mode with no SSID in EEPROM
  }
  // 2) else try and connect to the configured network
  else {
    WiFi.mode(WIFI_STA);
    wifi_mode = WIFI_MODE_CLIENT;
    startClient();
  }

  // Start hostname broadcast in STA mode
  if ((wifi_mode == WIFI_MODE_CLIENT || wifi_mode == WIFI_MODE_AP_AND_STA)) {
    if (MDNS.begin(esp_hostname)) {
      MDNS.addService("http", "tcp", 80);
    }
  }

  Timer = millis();
}

void wifi_loop() {
#ifdef WIFI_LED
  if (wifi_mode == WIFI_MODE_AP_ONLY && millis() > wifiLedTimeOut) {
    wifiLedState = !wifiLedState;
    digitalWrite(WIFI_LED, wifiLedState);
    wifiLedTimeOut = millis() + WIFI_LED_AP_TIME;
  }
#endif

  dnsServer.processNextRequest(); // Captive portal DNS re-dierct

  // Remain in AP mode for 5 Minutes before resetting
  if (wifi_mode == WIFI_MODE_AP_STA_RETRY) {
    if ((millis() - Timer) >= 300000) {
#ifdef ESP32
      esp_restart();
#else
      ESP.reset();
#endif
      DEBUG.println("WIFI Mode = 1, resetting");
    }
  }
}

void wifi_restart() {
  // Startup in STA + AP mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  // Create Unique SSID e.g "emonESP_XXXXXX"
#ifdef ESP32
  String softAP_ssid_ID =
    String(softAP_ssid) + "_" + String((uint32_t)ESP.getEfuseMac());
#else
  String softAP_ssid_ID =
    String(softAP_ssid) + "_" + String(ESP.getChipId());
#endif
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password);

  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  wifi_mode = WIFI_MODE_AP_AND_STA;
  startClient();
}

void wifi_scan() {
  DEBUG.println("WIFI Scan");
  int n = WiFi.scanNetworks();
  DEBUG.print(n);
  DEBUG.println(" networks found");
  st = "";
  rssi = "";
  for (int i = 0; i < n; ++i) {
    st += "\"" + WiFi.SSID(i) + "\"";
    rssi += "\"" + String(WiFi.RSSI(i)) + "\"";
    if (i < n - 1)
      st += ",";
    if (i < n - 1)
      rssi += ",";
  }
}

void wifi_disconnect() {
  WiFi.disconnect();
}
