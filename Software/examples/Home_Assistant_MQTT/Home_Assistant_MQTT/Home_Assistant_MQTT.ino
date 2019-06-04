#include <ArduinoJson.h>

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <WiFiClient.h>
#include <PubSubClient.h>  // MQTT https://github.com/knolleary/pubsubclient PlatformIO lib: 89
#include <SPI.h>
#include <ATM90E32.h> // for ATM90E32 energy meter

/***** WIFI SETTINGS *****/
const char* ssid = ""; //Your Network SSID
const char* password =  ""; //Your Network Password

/***** MQTT SETTINGS *****/
const char* mqttServer = ""; //Your MQTT Server
const int mqttPort = 1883; //Your MQTT Port
const char* mqttUser = ""; //Your MQTT User Name
const char* mqttPassword = ""; //Your MQTT Password

const char* HATopic = "home/energy/sensor"; //Home Assistant Topic - must change in sensor.yaml too

/***** CALIBRATION SETTINGS *****/
unsigned short lineFreq = 4485;         //4485 for 60 Hz (North America)
                                        //389 for 50 hz (rest of the world)
unsigned short PGAGain = 21;            //21 for 100A (2x), 42 for >100A (4x)

unsigned short VoltageGain = 42080;     //42080 - 9v AC transformer.
                                        //32428 - 12v AC Transformer
                                        
unsigned short CurrentGainCT1 = 38695;  //38695 - SCT-016 120A/40mA
unsigned short CurrentGainCT2 = 38695;  //25498 - SCT-013-000 100A/50mA
                                        //46539 - Magnalab 100A w/ built in burden resistor

#if defined ESP8266
const int CS_pin = 16;
/*
  D5/14 - CLK
  D6/12 - MISO
  D7/13 - MOSI
*/
#elif defined ESP32
const int CS_pin = 5;
/*
  18 - CLK
  19 - MISO
  23 - MOSI
*/

#elif defined ARDUINO_ESP8266_WEMOS_D1MINI  // WeMos mini and D1 R2
const int CS_pin = D8; // WEMOS SS pin

#elif defined ARDUINO_ESP8266_ESP12  // Adafruit Huzzah
const int CS_pin = 15; // HUZZAH SS pins ( 0 or 15)

#elif defined ARDUINO_ARCH_SAMD || defined __AVR_ATmega32U4__ //M0 board || 32u4 SS pin
const int CS_pin = 10; 

#else
const int CS_pin = SS; // Use default SS pin for unknown Arduino
#endif

//pass CS pin and calibrations to ATM90E32 library - the 2nd (B) current channel is not used with the split phase meter 
ATM90E32 eic(CS_pin, lineFreq, PGAGain, VoltageGain, CurrentGainCT1, 0, CurrentGainCT2); 

// Create client for MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  delay(2000);
 
  Serial.begin(115200);

  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("EnergyMeterClient", mqttUser, mqttPassword )) {
 
      Serial.println("connected");
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
 
  /* Initialise the ATM90E32 + SPI port */
  Serial.println("Start ATM90E32");
  eic.begin();
  delay(1000);
  
} // end setup
 
void loop() {
 
  StaticJsonDocument<256> meterData;
 
 /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, totalVoltage, currentCT1, currentCT2, totalCurrent, realPower, powerFactor, temp, freq, totalWatts;

  unsigned short sys0 = eic.GetSysStatus0(); //EMMState0
  unsigned short sys1 = eic.GetSysStatus1(); //EMMState1
  unsigned short en0 = eic.GetMeterStatus0();//EMMIntState0
  unsigned short en1 = eic.GetMeterStatus1();//EMMIntState1

  Serial.println("Sys Status: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
  Serial.println("Meter Status: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));
  delay(10);

  //if true the MCU is not getting data from the energy meter
  if (sys0 == 65535 || sys0 == 0) Serial.println("Error: Not receiving data from energy meter - check your connections");

  //get voltage
  voltageA = eic.GetLineVoltageA();
  voltageC = eic.GetLineVoltageC();

  if (lineFreq = 4485) {
    totalVoltage = voltageA + voltageC;     //is split single phase, so only 120v per leg
  }
  else {
    totalVoltage = voltageA;     //voltage should be 220-240 at the AC transformer
  }

  //get current
  currentCT1 = eic.GetLineCurrentA();
  currentCT2 = eic.GetLineCurrentC();
  totalCurrent = currentCT1 + currentCT2;

  realPower = eic.GetTotalActivePower();
  powerFactor = eic.GetTotalPowerFactor();
  temp = eic.GetTemperature();
  freq = eic.GetFrequency();
  totalWatts = (voltageA * currentCT1) + (voltageC * currentCT2);

  Serial.println("Voltage 1: " + String(voltageA) + "V");
  Serial.println("Voltage 2: " + String(voltageC) + "V");
  Serial.println("Current 1: " + String(currentCT1) + "A");
  Serial.println("Current 2: " + String(currentCT2) + "A");
  Serial.println("Active Power: " + String(realPower) + "W");
  Serial.println("Power Factor: " + String(powerFactor));
  Serial.println("Fundimental Power: " + String(eic.GetTotalActiveFundPower()) + "W");
  Serial.println("Harmonic Power: " + String(eic.GetTotalActiveHarPower()) + "W");
  Serial.println("Reactive Power: " + String(eic.GetTotalReactivePower()) + "var");
  Serial.println("Apparent Power: " + String(eic.GetTotalApparentPower()) + "VA");
  Serial.println("Phase Angle A: " + String(eic.GetPhaseA()));
  Serial.println("Chip Temp: " + String(temp) + "C");
  Serial.println("Frequency: " + String(freq) + "Hz");

  //slightly different way of encoding things using ArduinoJson 6
  meterData["V1"] = voltageA;
  meterData["V2"] = voltageC;
  meterData["I1"] = currentCT1;
  meterData["I2"] = currentCT2;
  meterData["totI"] = totalCurrent;
  meterData["W"] = totalWatts;
  meterData["AP"] = realPower;
  meterData["PF"] = powerFactor;
  meterData["t"] = temp;
  meterData["f"] = freq;
  
  char JSONmessageBuffer[200];
  serializeJson(meterData,JSONmessageBuffer);
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);

 // publish to topic
  if (client.publish(HATopic, JSONmessageBuffer) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }
 
  client.loop();
  Serial.println("-------------");
  
  delay(10000); //every 10 seconds
 
}

