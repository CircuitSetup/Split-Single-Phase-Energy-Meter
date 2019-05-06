/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor

   Modified to use CircuitSetup.us Energy Meter by jdeglavina
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
#include "config.h"
#include "wifi.h"
#include "web_server.h"
#include "ota.h"
#include "input.h"
#include "emoncms.h"
#include "mqtt.h"

// for ATM90E32 energy meter
#include <SPI.h>
#include <ATM90E32.h>

/***** CALIBRATION SETTINGS *****/
unsigned short LineGain = 7481; //7481 - 0x1D39 
unsigned short VoltageGain = 41820; //9v AC transformer. 
                                    //32428 - 12v AC Transformer
unsigned short CurrentGainCT1 = 25498;  //SCT-013-000 100A/50mA
                                        //46539 - Magnalab 100A
unsigned short CurrentGainCT2 = 25498;  //SCT-013-000 100A/50mA
                                        //46539 - Magnalab 100A
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


static const int LED_BUILTIN = 2;


ATM90E32 eic(CS_pin, LineGain, VoltageGain, CurrentGainCT1, CurrentGainCT2); //pass CS pin and calibrations to ATM90E32 library

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  delay(2000);

  Serial.begin(115200);
#ifdef DEBUG_SERIAL1
  Serial1.begin(115200);
#endif

  DEBUG.println();
  DEBUG.print("EmonESP ");
  DEBUG.println("Firmware: " + currentfirmware);

  // if there is an onboard LED, set it up
  pinMode(LED_BUILTIN, OUTPUT);

  // Read saved settings from the config
  config_load_settings();

  // Initialise the WiFi
  wifi_setup();

  // Bring up the web server
  web_server_setup();

  // Start the OTA update systems
  ota_setup();

  DEBUG.println("Server started");

  /*Initialise the ATM90E32 + SPI port */
  Serial.println("Start ATM90E32");
  eic.begin();
  delay(1000);


} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop()
{
  ota_loop();
  web_server_loop();
  wifi_loop();

  digitalWrite(LED_BUILTIN, HIGH);

  
  /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, totalVoltage, currentCT1, currentCT2, totalCurrent, realPower, powerFactor, temp, freq, totalWatts;

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
  currentCT1 = eic.GetLineCurrentA();
  // Current B is not used
  currentCT2 = eic.GetLineCurrentC();
  totalCurrent = currentCT1 + currentCT2;
  realPower = eic.GetTotalActivePower();
  powerFactor = eic.GetTotalPowerFactor();
  temp = eic.GetTemperature();
  freq = eic.GetFrequency();
  totalWatts = (voltageA * currentCT1) + (voltageC * currentCT2);

  Serial.println("VA:" + String(voltageA) + "V");
  Serial.println("VC:" + String(voltageC) + "V");
  Serial.println("IA:" + String(currentCT1) + "A");
  Serial.println("IC:" + String(currentCT2) + "A");
  Serial.println("AP:" + String(realPower));
  Serial.println("PF:" + String(powerFactor));
  Serial.println(String(temp) + "C");
  Serial.println("f" + String(freq) + "Hz");

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
  postStr += String(temp);
  postStr += ",W:";
  postStr += String(totalWatts);



  //boolean gotInput = input_get(postStr);

  //if (wifi_mode == WIFI_MODE_CLIENT || wifi_mode == WIFI_MODE_AP_AND_STA)
  //{
  //if (gotInput) { //(emoncms_apikey != 0 && gotInput) {
  Serial.println(postStr);
  emoncms_publish(postStr);
  //}
  /*
    if (mqtt_server != 0)
    {
    mqtt_loop();
    if (gotInput) {
      mqtt_publish(postStr);
    }
    }
  */
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  
  //}
} // end loop

