/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor

   Modified to use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

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
#include "mqtt.h"
#include "config.h"
#include "wifi.h"

#define MQTT_HOST mqtt_server.c_str()
#define MQTT_PORT 1883
#define MQTT_QOS 1
#define MQTT_CLEAN_SES true 

AsyncMqttClient mqttclient;   // Create client for MQTT

long lastMqttReconnectAttempt = 0;
int clientTimeout = 0;
int i = 0;

void onMqttConnect(bool sessionPresent) {
  DBUGS.println("<< MQTT connected >>");
  DBUGS.print("Session present: ");
  DBUGS.println(sessionPresent);
  uint16_t packetIdSub = mqttclient.subscribe(mqtt_topic.c_str(), MQTT_QOS);
  DBUGS.print("MQTT subscribed to ");
  DBUGS.print(mqtt_topic.c_str());
  DBUGS.print(", packetId: ");
  DBUGS.println(packetIdSub);
  mqttclient.publish(mqtt_topic.c_str(),  MQTT_QOS, MQTT_CLEAN_SES, "Energy monitor connected"); // Once connected, publish an announcement..
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  DBUGS.println("Disconnected from MQTT");
}

void onMqttPublish(uint16_t packetId) {
  DBUGS.print("MQTT published - packetID: ");
  DBUGS.println(packetId);
}

// -------------------------------------------------------------------
// MQTT Setup
// -------------------------------------------------------------------
void mqtt_setup()
{
  mqttclient.onConnect(onMqttConnect);
  mqttclient.onDisconnect(onMqttDisconnect);
  //mqttclient.onPublish(onMqttPublish);
  mqttclient.setKeepAlive(60);
  mqttclient.setServer(MQTT_HOST, MQTT_PORT);
  if (mqtt_user.length() != 0) { //if no username then connect anonymously, otherwise send credentials
     mqttclient.setCredentials(mqtt_user.c_str(), mqtt_pass.c_str());
  }
}


// -------------------------------------------------------------------
// MQTT Connect
// Called only when MQTT server field is populated
// -------------------------------------------------------------------
boolean mqtt_connect()
{
  DBUGS.println("MQTT connecting...");
  mqttclient.connect();
  delay(100);
  
  if (mqtt_connected()) {
    // see onMqttConnect
  }
  else {
    DBUGS.println("MQTT connection failed");
    return (0);
  }
  return (1);
}

// -------------------------------------------------------------------
// Publish to MQTT
// Split up data string into sub topics: e.g
// data = CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7
// base topic = emon/emonesp
// MQTT Publish: emon/emonesp/CT1 > 3935 etc..
// -------------------------------------------------------------------
void mqtt_publish(String data)
{
  String mqtt_data = "";
  String topic = mqtt_topic + "/" + mqtt_feed_prefix;
  int i = 0;
  while (int (data[i]) != 0)
  {
    // Construct MQTT topic e.g. <base_topic>/CT1 e.g. emonesp/CT1
    while (data[i] != ':') {
      topic += data[i];
      i++;
      if (int(data[i]) == 0) {
        break;
      }
    }
    i++;
    // Construct data string to publish to above topic
    while (data[i] != ',') {
      mqtt_data += data[i];
      i++;
      if (int(data[i]) == 0) {
        break;
      }
    }
    // send data via mqtt
    //DBUGS.printf("%s = %s\r\n", topic.c_str(), mqtt_data.c_str());
    if (!mqttclient.publish(topic.c_str(), MQTT_QOS, MQTT_CLEAN_SES, mqtt_data.c_str())) {
      return;
    }
    topic = mqtt_topic + "/" + mqtt_feed_prefix;
    mqtt_data = "";
    i++;
    if (int(data[i]) == 0) break;
  } 

  // send esp free ram
  String ram_topic = mqtt_topic + "/" + mqtt_feed_prefix + "freeram";
  String free_ram = String(ESP.getFreeHeap());
  if (!mqttclient.publish(ram_topic.c_str(), MQTT_QOS, MQTT_CLEAN_SES, free_ram.c_str())) {
    return;
  }

  // send wifi signal strength
  long rssi = WiFi.RSSI();
  String rssi_S = String(rssi);
  String rssi_topic = mqtt_topic + "/" + mqtt_feed_prefix + "rssi";
  if (!mqttclient.publish(rssi_topic.c_str(), MQTT_QOS, MQTT_CLEAN_SES, rssi_S.c_str())) {
    return;
  }
}

// -------------------------------------------------------------------
// MQTT state management
//
// Call every time around loop() if connected to the WiFi
// -------------------------------------------------------------------
void mqtt_loop()
{
  if (!mqttclient.connected()) {
    long now = millis();
    // initially try and reconnect continuously for 5s then try again once every 10s
    if ( (now < 5000) || ((now - lastMqttReconnectAttempt)  > 10000) ) {
      lastMqttReconnectAttempt = now;
      if (mqtt_connect()) { // Attempt to reconnect
        lastMqttReconnectAttempt = 0;
      }
    }
  } 
}

void mqtt_restart()
{
  if (mqtt_connected()) {
    mqttclient.disconnect();
  }
}

boolean mqtt_connected()
{
  return mqttclient.connected();
}
