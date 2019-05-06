/* ATM90E32 Energy Monitor Demo Application

   The MIT License (MIT)

  Copyright (c) 2016 whatnick and Ryzee

  Modified to use CircuitSetup.us Split Phase Energy Meter by jdeglavina

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <SPI.h>
#include <ATM90E32.h>

/***** CALIBRATION SETTINGS *****/
unsigned short LineGain = 7481; //0x1D39 
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

ATM90E32 eic(CS_pin, LineGain, VoltageGain, CurrentGainCT1, CurrentGainCT2); //pass CS pin and calibrations to ATM90E32 library

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  /* Initialize the serial port to host */
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("Start ATM90E32");

  /*Initialise the ATM90E32 + SPI port */
  eic.begin();
  delay(1000);

}

void loop() {

  /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, avgVoltage, currentA, currentC, totalCurrent, realPower, powerFactor, temp, freq;

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
  avgVoltage = (voltageA + voltageC) / 2;
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
  delay(1000);
}
