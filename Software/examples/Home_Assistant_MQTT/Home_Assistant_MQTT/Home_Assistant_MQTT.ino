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

/* Set this to 0 to only output to serial (for calibration tests) */
#define WIFI_CONNECT 1

/* Set this to 0 to connect to Wifi but not attempt an MQTT connection (for WiFi joining tests) */
#define MQTT_CONNECT 1

/* Number of initialization failures before the device rebotos and tries again. */
#define MAX_WIFI_INIT_FAILURES_BEFORE_REBOOT 20
#define MAX_MQTT_INIT_FAILURES_BEFORE_REBOOT 5

/* Number of metrics send failures before the device reboots and tries to re-init. */
#define MAX_ERRORS_BEFORE_REBOOT 3

/* If you have 200A clamps and use 1X gain, you will only end up with a range of 0-65.535A
 * for current. This isn't nearly enough. So, the datasheet recommends dividing the calibration
 * constants by some constant value and then multiplying the resulting measurements by the
 * same value to get around the issue. I recommend a value of 3.0 which gets you a range of
 * 0-196.61A. Not quite the full 200A but if you are pulling 196A on each leg of a 200A main
 * panel you have more important problems to worry about. Note that if you are using the preset
 * values below you can safely leave this at 1.0.
 */
#define CALIBRATION_MULTIPLIER 1.0

/***** WIFI SETTINGS *****/
const char* ssid = ""; //Your Network SSID
const char* password = ""; //Your Network Password

/***** MQTT SETTINGS *****/
const char* mqttServer = ""; //Your MQTT Server
const int mqttPort = 1883; //Your MQTT Port
const char* mqttUser = ""; //Your MQTT User Name
const char* mqttPassword = ""; //Your MQTT Password

const char* HATopic = "home/energy/sensor"; //Home Assistant Topic - must change in sensor.yaml too

/***** CALIBRATION SETTINGS *****/
/* 
 * 4485 for 60 Hz (North America)
 * 389 for 50 hz (rest of the world)
 */
unsigned short LineFreq = 4485;

/* 
 * 0 for 1x (CTs up to 60mA/720mV)
 * 21 for 2x (CTs up to 30mA/360mV)
 * 42 for 4x (CTs up to 15mA/180mV)
 */
unsigned short PGAGain = 21;

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

ATM90E32 eic{}; //initialize the IC class

// Create client for MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Count number of errors, for auto-reboot when MQTT server or WiFi go down.
int errorCount;

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  /* Initialize the error counter. */
  errorCount = 0;

  Serial.begin(115200);

  Serial.println("Booting up...");

#if WIFI_CONNECT
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
    errorCount ++;

    if (errorCount >= MAX_WIFI_INIT_FAILURES_BEFORE_REBOOT) {
      /* Possibly server is dead, reboot, reconnect to WiFi and try again. */
      ESP.restart();
    }
  }
 
  Serial.println("Connected to the WiFi network");
  errorCount = 0;

#if MQTT_CONNECT
  client.setServer(mqttServer, mqttPort);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("EnergyMeterClient", mqttUser, mqttPassword )) {
 
      Serial.println("Connected to MQTT");
      errorCount = 0;
 
    } else {
 
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(2000);
      
      errorCount++;
      if (errorCount >= MAX_MQTT_INIT_FAILURES_BEFORE_REBOOT) {
        /* Possibly server is dead, reboot, reconnect to WiFi and try again. */
        ESP.restart();
      }
    }
  }
#else
  Serial.println("MQTT disabled, using serial only...");
#endif
#else
  Serial.println("WiFi disabled, using serial only...");
#endif
 
  /*Initialise the ATM90E32 & Pass CS pin and calibrations to its library - 
   *the 2nd (B) current channel is not used with the split phase meter */
  Serial.println("Start ATM90E32");
  eic.begin(CS_pin, LineFreq, PGAGain, VoltageGain, CurrentGainCT1 / CALIBRATION_MULTIPLIER, 0, CurrentGainCT2 / CALIBRATION_MULTIPLIER);
  delay(1000);
  
} // end setup
 
void loop() {
 
  StaticJsonDocument<256> meterData;
 
 /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, currentCT1, currentCT2, totalCurrent, realPower, reactivePower, apparentPower, powerFactor, temp, freq, totalWatts;

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

  //get current
  currentCT1 = eic.GetLineCurrentA() * CALIBRATION_MULTIPLIER;
  currentCT2 = eic.GetLineCurrentC() * CALIBRATION_MULTIPLIER;
  totalCurrent = currentCT1 + currentCT2;

  realPower = eic.GetTotalActivePower() * CALIBRATION_MULTIPLIER;
  reactivePower = eic.GetTotalReactivePower() * CALIBRATION_MULTIPLIER;
  apparentPower = eic.GetTotalApparentPower() * CALIBRATION_MULTIPLIER;
  powerFactor = eic.GetTotalPowerFactor();
  temp = eic.GetTemperature();
  freq = eic.GetFrequency();

  Serial.println("Voltage 1: " + String(voltageA) + "V");
  Serial.println("Voltage 2: " + String(voltageC) + "V");
  Serial.println("Current 1: " + String(currentCT1) + "A");
  Serial.println("Current 2: " + String(currentCT2) + "A");
  Serial.println("Apparent Power: " + String(apparentPower) + "W");
  Serial.println("Real Power: " + String(realPower) + "W");
  Serial.println("Reactive Power: " + String(reactivePower) + "var");
  Serial.println("Power Factor: " + String(powerFactor) + "%");
  Serial.println("Phase Angle A: " + String(eic.GetPhaseA()));
  Serial.println("Chip Temp: " + String(temp) + "C");
  Serial.println("Frequency: " + String(freq) + "Hz");

#if WIFI_CONNECT
#if MQTT_CONNECT
  //slightly different way of encoding things using ArduinoJson 6
  meterData["V1"] = voltageA;
  meterData["V2"] = voltageC;
  meterData["I1"] = currentCT1;
  meterData["I2"] = currentCT2;
  meterData["totI"] = totalCurrent;
  meterData["AP"] = apparentPower;
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
    errorCount = 0;
  } else {
    Serial.println("Error sending message");
    errorCount++;

    if (errorCount >= MAX_ERRORS_BEFORE_REBOOT) {
      /* The WiFi connection died, or MQTT died, reboot and try again. */
      ESP.restart();
    }
  }
 
  client.loop();
  Serial.println("-------------");
#endif
#endif
  
  delay(10000); //every 10 seconds
 
}
