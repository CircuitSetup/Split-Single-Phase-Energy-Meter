/* ATM90E32 Energy Monitor Demo Application

   The MIT License (MIT)

  Copyright (c) 2016 whatnick and Ryzee
  Modified by jdeglavina for CircuitSetup Energy Meters

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <FS.h> //this needs to be first, or it all crashes and burns..
#include <SPIFFS.h>


#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
#endif
#ifdef ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#endif
#include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson

//flag for saving data
bool shouldSaveConfig = true;

#include <Wire.h>
#include <SPI.h>
#include <ATM90E32.h>

/*
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <pgmspace.h>

//pins for the OLED SPI display (uses software SPI)
#define OLED_CLK   18 //D0 
#define OLED_MOSI  23 //D1
#define OLED_RST   17
#define OLED_DC    4
#define OLED_CS    15 //must be different for each device

//if only DC, RST, and CS are passed, this indicates hardware SPI
//CLK & MOSI are assumed to be default pins
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);
*/


char server[50] = "api.thingspeak.com";
// Sign up on thingspeak and get WRITE API KEY.
char auth[36] = "THINGSPEAK_KEY";

WiFiClient client;

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

unsigned short VoltageGain = 3920;
unsigned short CurrentGainCT1 = 39473;
unsigned short CurrentGainCT2 = 39473;
unsigned short LineFreq = 4485;
unsigned short PGAGain = 0;


ATM90E32 eic{}; //initialize the IC class

void setup() {
  /* Initialize the serial port to host */
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }

/*
  Wire.begin();

  display.begin(SSD1306_SWITCHCAPVCC);

  // Clear the buffer.
  display.clearDisplay();
  display.setCursor(0, 0);

  display.setTextSize(1.5);
  display.setTextColor(WHITE);
*/

  //Read previous config
  readTSConfig();

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_ts_token("ts", "V9UNFM19EHTW1430", auth, 33);
  WiFiManagerParameter custom_server("serv", "api.thingspeak.com", server, 50);

  //Use wifi manager to get config
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_ts_token);
  wifiManager.addParameter(&custom_server);


  //display.println("Connecting to Wifi ");
  //display.display();

  //first parameter is name of access point, second is the password
  wifiManager.autoConnect("AP_NAME", "PASSWORD");

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  Serial.print("Key:");
  Serial.println(auth);
  Serial.print("Server:");
  Serial.println(server);

/*
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Wifi Connected!!");
  display.display();
  delay(1000);
  */

  //read updated parameters
  strcpy(auth, custom_ts_token.getValue());
  strcpy(server, custom_server.getValue());

  saveTSConfig();

  /*Initialise the ATM90E32 + SPI port */
  Serial.println("Start ATM90E32");
  eic.begin(CS_pin, LineFreq, PGAGain, VoltageGain, CurrentGainCT1, 0, CurrentGainCT2);
  delay(1000);
}

void loop() {

  /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, totalVoltage, currentA, currentC, totalCurrent, realPower, powerFactor, temp, freq;

  unsigned short sys0 = eic.GetSysStatus0();
  unsigned short sys1 = eic.GetSysStatus1();
  unsigned short en0 = eic.GetMeterStatus0();
  unsigned short en1 = eic.GetMeterStatus1();

  Serial.println("Sys Status: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
  Serial.println("Meter Status: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));
  delay(10);

  voltageA = eic.GetLineVoltageA();
  // Voltage B is not used
  voltageC = eic.GetLineVoltageC();
  totalVoltage = voltageA + voltageC ;
  currentA = eic.GetLineCurrentA();
  // Current B is not used
  currentC = eic.GetLineCurrentC();
  totalCurrent = currentA + currentC;
  realPower = eic.GetTotalActivePower();
  powerFactor = eic.GetTotalPowerFactor();
  temp = eic.GetTemperature();
  freq = eic.GetFrequency();

  Serial.println("VA:" + String(voltageA) + "V");
  Serial.println("VC:" + String(voltageC) + "V");
  Serial.println("IA:" + String(currentA) + "A");
  Serial.println("IC:" + String(currentC) + "A");
  Serial.println("AP:" + String(realPower));
  Serial.println("PF:" + String(powerFactor));
  Serial.println(String(temp) + "C");
  Serial.println("f" + String(freq) + "Hz");


  if (client.connect(server, 80)) { //   "184.106.153.149" or api.thingspeak.com
    String postStr = String(auth);
    postStr += "&field1=";
    postStr += String(voltageA);
    postStr += "&field2=";
    postStr += String(voltageC);
    postStr += "&field3=";
    postStr += String(currentA);
    postStr += "&field4=";
    postStr += String(currentC);
    postStr += "&field5=";
    postStr += String(realPower);
    postStr += "&field6=";
    postStr += String(powerFactor);
    postStr += "&field7=";
    postStr += String(temp);
    postStr += "&field8=";
    postStr += String(totalCurrent);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + String(auth) + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
  client.stop();
  
/*
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Voltage: ");
  display.print(avgVoltage);
  display.println("V");
  display.print("Current: ");
  display.print(totalCurrent);
  display.println("A");
  display.print("Power: ");
  display.println(realPower);
  display.print("Temp: ");
  display.print(temp);
  display.println("C");

  display.display();
  */

  delay(1000);
  delay(14000);

/*
  //Clear display to prevent burn in
  display.clearDisplay();
  display.display();
*/
}


//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void readTSConfig()
{
  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          //Serial.println("\nparsed json");
          strcpy(auth, json["auth"]);
          strcpy(server, json["server"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}

void saveTSConfig()
{
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["auth"] = auth;
    json["server"] = server;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
}
