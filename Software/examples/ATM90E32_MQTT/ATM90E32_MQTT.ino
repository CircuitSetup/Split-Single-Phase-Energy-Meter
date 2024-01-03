// by digiblur
// https://github.com/digiblur/digiNRG_SplitPhase

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>  // https://github.com/marvinroger/async-mqtt-client (requires: https://github.com/me-no-dev/ESPAsyncTCP)
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "config.h"           // rename sample_config.h and edit any values needed
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

// for ATM90E32 board
#include <SPI.h>
#include <ATM90E32.h> 

#ifdef ESP8266
const int CS_pin = 04; 
/*
  D0/16 - CS
  D8/15 - CS  // wemos d1
  D5/14 - CLK
  D6/12 - MISO
  D7/13 - MOSI
*/
#endif

/***** CALIBRATION SETTINGS *****/
/* 
 * 4485 for 60 Hz (North America)
 * 389 for 50 hz (rest of the world)
 */
unsigned short lineFreq = 4485;         

/* 
 * 0 for 1x (CTs up to 60mA/720mV)
 * 21 for 2x (CTs up to 30mA/360mV)
 * 42 for 4x (CTs up to 15mA/180mV)
 */
unsigned short PGAGain = 0;            

/* 
 * For meter <= v1.3:
 *    42080 - 9v AC Transformer - Jameco 112336
 *    32428 - 12v AC Transformer - Jameco 167151
 * For meter > v1.4:
 *    37106 - 9v AC Transformer - Jameco 157041
 *    38302 - 9v AC Transformer - Jameco 112336
 *    29462 - 12v AC Transformer - Jameco 167151
 * For Meters > v1.4 purchased after 11/1/2019 and rev.3
 *    3920 - 9v AC Transformer - Jameco 157041
 */
unsigned short VoltageGain = 3920;     
                                       
/*
 * 25498 - SCT-013-000 100A/50mA
 * 39473 - SCT-016 120A/40mA
 * 46539 - Magnalab 100A
 */                                  
unsigned short CurrentGainCT1 = 39473;  
unsigned short CurrentGainCT2 = 39473;  
                                        


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Layout (Comment Only)                                         //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// D0/16 -            (NodeMCU has secondary LED)
// D1/05 - Digital #1 In
// D2/04 -
// D3/00 -                         Wemos 10k Pull-Up
// D4/02 - Wemos D1 & NodeMCU Onboard LED - 10k Pull-Up
// D5/14 - AT90 - CLK
// D6/12 - AT90 - MISO
// D7/13 - AT90 - MOSI
// D8/15 - AT90 - CS         Wemos 10k Pull-Down

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Input Pins                                                    //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
#define digIn1Pin     D1
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Output Pins                                                   //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
#define intLED1Pin  D4  // D4 Wemos Mini D1 and NodeMCU
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Verb/State Conversions                                        //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
#define LEDon       LOW
#define LEDoff      HIGH
#define digLowStr   "Low"
#define digHighStr  "High"
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Poll Times / Misc                                             //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
#define ATM90Poll      5   // Poll ATM90E32 Time
#define rePushPoll     300 // repush all sensor data every xx seconds
#define mqttQOSLevel   2
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// VARS Begin                                                    //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
char mcuHostName[64]; 
char digIn1Topic[96];    
char lwtTopic[96];  
char powerTopic[96];
char buildTopic[96];
char rssiTopic[96];
int digIn1Value;
int digIn1Status;
char mqtt_data[512];
float voltageA, voltageC, totalVoltage, currentCT1, currentCT2, totalCurrent, realPower, powerFactor, chipTemp, powerFreq, totalWatts;
unsigned short sys0, sys1, en0, en1;
unsigned long wifiLoopNow = 0;
int mqttTryCount = 0;

bool initBoot = true;
bool readATM90 = false;

ATM90E32 eic{}; //initialize the IC class

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
#ifdef DEBUGTELNET
  WiFiServer telnetServer(23);
  WiFiClient telnetClient;
#endif

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
Ticker ATM90Tick;
Ticker rePushTick;
Ticker led1FlipTick;
Ticker wifiReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// connectToWifi                                                 //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void connectToWifi() {
  debugLn(F("WIFI: Attempting to Connect."));
  wifiLoopNow = millis(); // mark start time
  WiFi.mode(WIFI_STA);
  WiFi.hostname(mcuHostName);
  WiFi.begin(wifiSSID, wifiPass);
  delay(10);  
  // toggle on board LED as WiFi comes up
  digitalWrite(intLED1Pin, LEDoff);
  while (WiFi.status() != WL_CONNECTED) {
     digitalWrite(intLED1Pin, !(digitalRead(intLED1Pin)));  //Invert Current State of LED  
     delay(70);
     digitalWrite(intLED1Pin, !(digitalRead(intLED1Pin)));   
     delay(45);
     if (millis() > wifiLoopNow + 30000) {  // WiFi Stuck?  Reboot ESP and start over...
       debugLn(F("ESP: WiFi Failed. Restarting ESP."));
       delay(100);
       ESP.restart();      
     }   
  }
  digitalWrite(intLED1Pin, LEDoff);
}
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// onWifiConnect                                                 //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  long rssi = WiFi.RSSI()*-1;
  debugLn(String(F("WIFI: Connected - IP: ")) + WiFi.localIP().toString() + " - RSSI: " + String(rssi) );
  connectToMqtt();
}
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// onWifiDisconnect                                              //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  debugLn(F("WIFI: Disconnected."));
  mqttReconnectTimer.detach(); // don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// connectToMqtt                                                 //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void connectToMqtt() {
  debugLn(String(F("MQTT: Attempting connection to ")) + String(mqttHost) + " as " + mcuHostName);
  mqttClient.connect();
  mqttTryCount++;
  if (mqttTryCount > 15) {
    debugLn(F("ESP: MQTT Failed too many times. Restarting ESP."));
    delay(100);
    ESP.restart();      
  }  
}
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
bool checkBoundSensor(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// onMqttConnect                                                 //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void onMqttConnect(bool sessionPresent) {
  debugLn(F("MQTT: Connected"));
  mqttTryCount = 0;
  mqttClient.publish(lwtTopic, 2, true, mqttBirth);
  // Setup Sensor Polling
  ATM90Tick.attach(ATM90Poll, getATM90);
  rePushTick.attach(rePushPoll, rePushVals);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// onMqttDisconnect                                              //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  debugLn(F("MQTT: Disconnected."));
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
  // if MQTT is disconnected stop polling sensors
  ATM90Tick.detach();
  rePushTick.detach();
}

void flipLED1() {
  digitalWrite(intLED1Pin, !(digitalRead(intLED1Pin)));  //Invert Current State of LED  
}

const char *getDeviceID() {
  char *identifier = new char[30];
  os_strcpy(identifier, hostName);
  strcat_P(identifier, PSTR("-"));

  char cidBuf[7];
  sprintf(cidBuf, "%06X", ESP.getChipId());
  os_strcat(identifier, cidBuf);

  return identifier;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// ESP Setup                                                     //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void setup() {
  #ifdef DEBUGSERIAL
    Serial.begin(115200);
    while(!Serial) {} // Wait
    Serial.println();
  #endif  
  debugLn(String(F("digiNRG - Build: ")) + F(__DATE__) + " " +  F(__TIME__));
  // build hostname with last 6 of MACID
  os_strcpy(mcuHostName, getDeviceID());

  // ~~~~ Set MQTT Topics
  sprintf_P(lwtTopic, PSTR("%s/LWT"), mcuHostName);
  sprintf_P(digIn1Topic, PSTR("%s/digIn1"), mcuHostName);
  sprintf_P(rssiTopic, PSTR("%s/RSSI"), mcuHostName);
  sprintf_P(buildTopic, PSTR("%s/BUILD"), mcuHostName);
/*  sprintf_P(bmeTempTopic, PSTR("%s/BME280/Temp"), mcuHostName);
  sprintf_P(bmeHumiTopic, PSTR("%s/BME280/Humi"), mcuHostName);
  sprintf_P(bmePresTopic, PSTR("%s/BME280/Pres"), mcuHostName);
  sprintf_P(bmeFeelTopic, PSTR("%s/BME280/Feel"), mcuHostName); */
  sprintf_P(powerTopic, PSTR("%s/ATM90"), mcuHostName);
 
  // ~~~~ Set PIN Modes
  pinMode(intLED1Pin,OUTPUT); 
  pinMode(digIn1Pin,     INPUT);  // no pull-up with AM312

  digitalWrite(intLED1Pin, LEDoff);
  delay(10);  

  led1FlipTick.attach(0.4, flipLED1);
  debugLn(F("ATM90: Start"));
  /*Initialise the ATM90E32 & Pass CS pin and calibrations to its library - 
   *the 2nd (B) current channel is not used with the split phase meter */
  eic.begin(CS_pin, LineFreq, PGAGain, VoltageGain, CurrentGainCT1, 0, CurrentGainCT2);
  
  delay(500);  // not sure if needed - old code had 1000 after
  led1FlipTick.detach();
  digitalWrite(intLED1Pin, LEDoff);
  delay(10);  

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  // setup MQTT
  mqttClient.setWill(lwtTopic,2,true,mqttDeath,0);
  mqttClient.setCredentials(mqttUser,mqttPass);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setServer(mqttHost, mqttPort);
  mqttClient.setMaxTopicLength(512);
  mqttClient.setClientId(mcuHostName);

  connectToWifi();

  #ifdef DEBUGTELNET
    // Setup telnet server for remote debug output
    telnetServer.setNoDelay(true);
    telnetServer.begin();
    debugLn(String(F("Telnet: Started on port 23 - IP:")) + WiFi.localIP().toString());
  #endif

  // OTA Flash Sets
  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(mcuHostName);
  ArduinoOTA.setPassword((const char *)OTApassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Setup HTTP Flash Page
  httpUpdater.setup(&httpServer, "/flash", update_username, update_password);
  httpServer.on("/restart", []() {
    debugLn(F("HTTP: Restart request received."));
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.send(200, "text/plain", "Restart command sent to ESP MCU..." );
    delay(100);
    ESP.restart();
  });

  httpServer.begin();
  debugLn(F("ESP: Boot completed - Starting loop"));
}
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// rePushVals                                                    //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void rePushVals() {
    debugLn(F("ESP: rePushingVals to MQTT"));
    mqttClient.publish(rssiTopic, mqttQOSLevel, false, String(WiFi.RSSI()*-1).c_str()); 
    mqttClient.publish(buildTopic, mqttQOSLevel, false, String(String(F("digiNRG - Build: ")) + F(__DATE__) + " " +  F(__TIME__) + " - " + WiFi.localIP().toString()).c_str()); 
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// checkdigIn                                                    //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void checkdigIn() {
  digIn1Value = digitalRead(digIn1Pin); //read state of the digital pin
  // AM312 goes LOW with no motion
  if (digIn1Value == LOW && digIn1Status != 0) {
    debugLn(F("DIG1: Digital1 went Low - Publish to MQTT"));
    mqttClient.publish(digIn1Topic, mqttQOSLevel, false, digLowStr);
    digIn1Status = 0;
  }
  else if (digIn1Value == HIGH && digIn1Status != 1) {
    digitalWrite(intLED1Pin, LEDon);
    debugLn(F("DIG1: Digital1 went High - Publish to MQTT"));
    mqttClient.publish(digIn1Topic, mqttQOSLevel, false, digLowStr);
    digIn1Status = 1;
  }
}
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Telnet                                                        //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
#ifdef DEBUGTELNET
void handleTelnetClient()
{ 
  if (telnetServer.hasClient())
  {
    // client is connected
    if (!telnetClient || !telnetClient.connected())
    {
      if (telnetClient)
        telnetClient.stop();                   // client disconnected
      telnetClient = telnetServer.available(); // ready for new client
    }
    else
    {
      telnetServer.available().stop(); // have client, block new connections
    }
  }
  // Handle client input from telnet connection.
  if (telnetClient && telnetClient.connected() && telnetClient.available())
  {
    // client input processing
    while (telnetClient.available())
    {
      // Read data from telnet just to clear out the buffer
      telnetClient.read();
    }
  }
}
#endif

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Serial and Telnet Log Handler                                 //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void debugLn(String debugText)
{ 
  String debugTimeText = "[+" + String(float(millis()) / 1000, 3) + "s] " + debugText;
  #ifdef DEBUGSERIAL
    Serial.println(debugTimeText);
    Serial.flush();
  #endif
  #ifdef DEBUGTELNET
    if (telnetClient.connected())
    {
      debugTimeText += "\r\n";
      const size_t len = debugTimeText.length();
      const char *buffer = debugTimeText.c_str();
      telnetClient.write(buffer, len);
      handleTelnetClient();
    }
  #endif
}

void pushNRGmqtt() {

  String mqtt_dataa = String(F("{\"VA\":")) + String(voltageA) + String(F(",\"VC\":")) + String(voltageC) + String(F(",\"CT1A\":")) + String(currentCT1) + String(F(",\"CT2A\":")) + String(currentCT2) + String(F(",\"RP\":"))
              + String(realPower) + String(F(",\"PF\":")) + String(powerFactor) + String(F(",\"Temp\":")) + String(chipTemp) + String(F(",\"Freq\":")) + String(powerFreq) + String(F(",\"TotVolt\":"))
              + String(totalVoltage) + String(F(",\"TotAmp\":")) + String(totalCurrent) + String(F(",\"TotWatt\":")) + String(totalWatts) + String(F("}"));
 
  debugLn(String(mqtt_dataa));
  
  mqttClient.publish(powerTopic, mqttQOSLevel, false, String(mqtt_dataa).c_str());  
/*  mqttClient.publish(String(powerTopic+String("VC")).c_str(), mqttQOSLevel, false, String(voltageC).c_str());  
  mqttClient.publish(String(powerTopic+String("CT1A")).c_str(), mqttQOSLevel, false, String(currentCT1).c_str());  
  mqttClient.publish(String(powerTopic+String("CT2A")).c_str(), mqttQOSLevel, false, String(currentCT2).c_str());  
  mqttClient.publish(String(powerTopic+String("RP")).c_str(), mqttQOSLevel, false, String(realPower).c_str());  
  mqttClient.publish(String(powerTopic+String("PF")).c_str(), mqttQOSLevel, false, String(powerFactor).c_str());  
  mqttClient.publish(String(powerTopic+String("Temp")).c_str(), mqttQOSLevel, false, String(chipTemp).c_str());  
  mqttClient.publish(String(powerTopic+String("Freq")).c_str(), mqttQOSLevel, false, String(powerFreq).c_str());  
  mqttClient.publish(String(powerTopic+String("TotVolt")).c_str(), mqttQOSLevel, false, String(totalVoltage).c_str());  
  mqttClient.publish(String(powerTopic+String("TotAmp")).c_str(), mqttQOSLevel, false, String(totalCurrent).c_str());  
  mqttClient.publish(String(powerTopic+String("TotWatt")).c_str(), mqttQOSLevel, false, String(totalWatts).c_str());  
*/
}

void pushNRGhttp() {
  String postStr = "VA:";
  postStr += String(voltageA);
  postStr += ",VC:";
  postStr += String(voltageC);
  postStr += ",totV:";
  postStr += String(totalVoltage);
  postStr += ",IA:";
  postStr += String(currentCT1);
  postStr += ",IC:";
  postStr += String(currentCT2);
  postStr += ",totI:";
  postStr += String(totalCurrent);
  postStr += ",AP:";
  postStr += String(realPower);
  postStr += ",PF:";
  postStr += String(powerFactor);
  postStr += ",t:";
  postStr += String(chipTemp);
  postStr += ",W:";
  postStr += String(totalWatts);  
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// get Energy Vals                                               //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void getATM90() {

  readATM90 = true;

  sys0 = eic.GetSysStatus0();
  sys1 = eic.GetSysStatus1();
  en0 = eic.GetMeterStatus0();
  en1 = eic.GetMeterStatus1();

  debugLn(String(F("ATM90: Sys Status: S0:0x")) + String(sys0, HEX) + String(F(" S1:0x")) + String(sys1, HEX));
  debugLn(String(F("ATM90: Meter Status: E0:0x")) + String(en0, HEX) + String(F(" E1:0x")) + String(en1, HEX));

  voltageA    = eic.GetLineVoltageA();  // Voltage B is not used
  voltageC    = eic.GetLineVoltageC();
  currentCT1  = eic.GetLineCurrentA();  // Current B is not used
  currentCT2  = eic.GetLineCurrentC();
  realPower   = eic.GetTotalActivePower();
  powerFactor = eic.GetTotalPowerFactor();
  chipTemp    = eic.GetTemperature()*9.0/5.0 + 32.0; //Converts to F
  powerFreq   = eic.GetFrequency();

  totalVoltage = voltageA + voltageC ;
  totalCurrent = currentCT1 + currentCT2;
  totalWatts  = (voltageA * currentCT1) + (voltageC * currentCT2);

  readATM90 = false;

  debugLn(String(F("ATM90: VA: ")) + String(voltageA) + String(F("V - VC: ")) + String(voltageC) + String(F("V")));
  debugLn(String(F("ATM90: CT1-A: ")) + String(currentCT1) + String(F("A - CT2-A: ")) + String(currentCT2) + String(F("A")));
  debugLn(String(F("ATM90: RP: ")) + String(realPower) + String(F(" - PF: ")) + String(powerFactor));
  debugLn(String(F("ATM90: ATM90Temp: ")) + String(chipTemp) + String(F("C - Freq: ")) + String(powerFreq)+ String(F("hz")));
  debugLn(String(F("ATM90: TotalV: ")) + String(totalVoltage) + String(F("V - TotalA: ")) + String(totalCurrent) + String(F("A - TotalW: ")) + String(totalWatts) + String(F("W")));

  pushNRGmqtt();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// ESP Loop                                                      //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
void loop() {
  ArduinoOTA.handle();
  httpServer.handleClient();
  #ifdef DEBUGTELNET
    handleTelnetClient();
  #endif
  checkdigIn();
  if (initBoot) {   // on first loop pull sensors
    delay(2000);  // hold up before first pull, had exception issues in mqtt connect on early pulls
    initBoot = false;
    delay(20);
    if (WiFi.isConnected() && mqttClient.connected()) {
      mqttClient.publish(rssiTopic, mqttQOSLevel, false, String(WiFi.RSSI()*-1).c_str()); 
      mqttClient.publish(buildTopic, mqttQOSLevel, false, String(String(F("digiNRG - Build: ")) + F(__DATE__) + " " +  F(__TIME__) + " - " + WiFi.localIP().toString()).c_str()); 
      debugLn(String(F("digiNRG - Build: ")) + F(__DATE__) + " " +  F(__TIME__) + " - " + WiFi.localIP().toString());
    } 
  }
} 
