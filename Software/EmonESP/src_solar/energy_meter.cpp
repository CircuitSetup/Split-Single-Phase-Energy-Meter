/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Created for use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

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

#include "energy_meter.h"
#include "emonesp.h"
#include "emoncms.h"
#include "input.h"
#include "config.h"

// for ATM90E32 energy meter
#include <SPI.h>
#include <ATM90E32.h>

#ifdef ENABLE_OLED_DISPLAY
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_DC     0
#define OLED_CS     16
#define OLED_RESET  2 //17
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);
#endif

/***** CALIBRATION SETTINGS *****/
/* These values are edited in the web interface or energy_meter.h */
/* Values in the web interface take priority */
unsigned short VoltageGain = VOLTAGE_GAIN;
unsigned short CurrentGainCT1 = CURRENT_GAIN_CT1;
unsigned short CurrentGainCT2 = CURRENT_GAIN_CT2;
unsigned short LineFreq = LINE_FREQ;
unsigned short PGAGain = PGA_GAIN;
#ifdef SOLAR_METER
unsigned short VoltageGainSolar = VOLTAGE_GAIN_SOLAR;
unsigned short SolarGainCT1 = SOLAR_GAIN_CT1;
unsigned short SolarGainCT2 = SOLAR_GAIN_CT2;
#endif

#if defined ESP8266
const int CS_pin = 16;
/*
  D5/14 - CLK
  D6/12 - MISO
  D7/13 - MOSI
*/
#elif defined ESP32
const int CS_pin = 5;
#ifdef SOLAR_METER
const int CS_solar_pin = 4;
#endif
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

unsigned long startMillis;
unsigned long currentMillis;

const int period = 1000; //time interval in ms to send data
#ifdef SOLAR_METER
bool canBeNegative = true;
#else
bool canBeNegative = false; //set to true if current and power readings can be negative (like when exporting solar power)
#endif

char result[200];
char measurement[16];

ATM90E32 eic{}; //initialize the IC class
#ifdef SOLAR_METER
ATM90E32 eic_solar {};
#endif

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void energy_meter_setup() {

  /*Get values from web interface and assign them if populated*/
  if (voltage_cal.toInt() > 0) VoltageGain = voltage_cal.toInt();
  if (ct1_cal.toInt() > 0) CurrentGainCT1 = ct1_cal.toInt();
  if (ct2_cal.toInt() > 0) CurrentGainCT2 = ct2_cal.toInt();
  if (freq_cal.toInt() > 0) LineFreq = freq_cal.toInt();
  if (gain_cal.toInt() > 0) PGAGain = gain_cal.toInt();

  /*Initialise the ATM90E32 & Pass CS pin and calibrations to its library -
    the 2nd (B) current channel is not used with the split phase meter */
  Serial.println("Start ATM90E32");
  eic.begin(CS_pin, LineFreq, PGAGain, VoltageGain, CurrentGainCT1, 0, CurrentGainCT2);
  delay(1000);

#ifdef SOLAR_METER
  if (svoltage_cal.toInt() > 0) VoltageGainSolar = svoltage_cal.toInt();
  if (sct1_cal.toInt() > 0) SolarGainCT1 = sct1_cal.toInt();
  if (sct2_cal.toInt() > 0) SolarGainCT2 = sct2_cal.toInt();

  eic_solar.begin(CS_solar_pin, LineFreq, PGAGain, VoltageGainSolar, SolarGainCT1, 0, SolarGainCT2);
#endif

#ifdef ENABLE_OLED_DISPLAY
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("OLED allocation failed"));
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Starting up...");
  display.display();
#endif

  startMillis = millis();  //initial start time

} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void energy_meter_loop()
{
  /*get the current "time" (actually the number of milliseconds since the program started)*/
  currentMillis = millis();

  if (startMillis == 0) {
    startMillis = currentMillis;
    return;
  }
  if (currentMillis - startMillis < period)  //test whether the period has elapsed
  {
    return;
  }
  startMillis = currentMillis;

  /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, totalVoltage;
  float currentCT1, currentCT2, totalCurrent;
  float totalWatts, wattsA, wattsC;
  float powerFactor, temp, freq;

  unsigned short sys0 = eic.GetSysStatus0(); //EMMState0
  unsigned short sys1 = eic.GetSysStatus1(); //EMMState1
  unsigned short en0 = eic.GetMeterStatus0();//EMMIntState0
  unsigned short en1 = eic.GetMeterStatus1();//EMMIntState1

  DBUGS.println("Sys Status: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
  DBUGS.println("Meter Status: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));
  delay(10);

  /*if true the MCU is not getting data from the energy meter */
  if (sys0 == 65535 || sys0 == 0) DBUGS.println("Error: Not receiving data from energy meter - check your connections");

  ////// VOLTAGE
  if (LineFreq == 389) {
    //if 50hz then voltage is 240v and needs to be scaled
    voltageA = eic.GetLineVoltageA()*2;
    voltageC = eic.GetLineVoltageC()*2;
  }
  else {  
    voltageA = eic.GetLineVoltageA();
    voltageC = eic.GetLineVoltageC();

    totalVoltage = voltageA + voltageC;  
  }   

  /////// CURRENT & POWER
  currentCT1 = eic.GetLineCurrentA();
  currentCT2 = eic.GetLineCurrentC();

  if (LineFreq == 389) {
    //voltage is not changed in the register so this has to be accounted for here as well
    wattsA = eic.GetActivePowerA()*2; 
    wattsC = eic.GetActivePowerC()*2;
  }
  else {
    wattsA = eic.GetActivePowerA(); 
    wattsC = eic.GetActivePowerC();
  }

  if (canBeNegative) {
    /* Is net energy positive or negative?
       We're not reading if current is pos or neg because we're only getting
       the upper 16 bit register. We are getting all 32 bits of active power though */
    if (wattsA < 0) currentCT1 *= -1;
    if (wattsC < 0) currentCT2 *= -1;
    if (LineFreq == 389) {
      totalWatts = eic.GetTotalActivePower()*2; //to accoutn for 240v
    }
    else {
      totalWatts = eic.GetTotalActivePower(); //all math is already done in the total register
    }
  }
  else {
    /* If single split phase & 1 voltage reading, phase will be offset between the 2 phases
       making one power reading negative. This will correct for that negative reading. */
    if (wattsA < 0) wattsA *= -1;
    if (wattsC < 0) wattsC *= -1;
    totalWatts = wattsA + wattsC;
  }

  totalCurrent = currentCT1 + currentCT2;
  powerFactor = eic.GetTotalPowerFactor();

  /////// OTHER
  temp = eic.GetTemperature();
  freq = eic.GetFrequency();


#ifdef SOLAR_METER
  float solarVoltageA, solarVoltageC, totalSolarVoltage;
  float solarCurrentCT1, solarCurrentCT2, totalSolarCurrent;
  float totalSolarWatts, solarWattsA, solarWattsC;
  float solarPowerFactor, solarTemp, solarFreq;

  unsigned short sys0s = eic_solar.GetSysStatus0(); //EMMState0
  unsigned short sys1s = eic_solar.GetSysStatus1(); //EMMState1
  unsigned short en0s = eic_solar.GetMeterStatus0();//EMMIntState0
  unsigned short en1s = eic_solar.GetMeterStatus1();//EMMIntState1

  DBUGS.println("Solar Sys Status: S0:0x" + String(sys0s, HEX) + " S1:0x" + String(sys1s, HEX));
  DBUGS.println("Solar Meter Status: E0:0x" + String(en0s, HEX) + " E1:0x" + String(en1s, HEX));
  delay(10);

  ////// SOLAR VOLTAGE
  /*if true the MCU is not getting data from the energy meter */
  if (sys0s == 65535 || sys0s == 0) DBUGS.println("Error: Not receiving data from the solar energy meter - check your connections");

  if (LineFreq == 389) {
    //if 50hz then voltage is 240v and needs to be scaled
    solarVoltageA = eic_solar.GetLineVoltageA()*2;
    solarVoltageC = eic_solar.GetLineVoltageC()*2;
  }
  else {
    solarVoltageA = eic_solar.GetLineVoltageA();
    solarVoltageC = eic_solar.GetLineVoltageC();

    totalSolarVoltage = solarVoltageA + solarVoltageC;   
  }
  
  /////// SOLAR CURRENT & POWER
  solarCurrentCT1 = eic_solar.GetLineCurrentA();
  solarCurrentCT2 = eic_solar.GetLineCurrentC();

  if (LineFreq == 389) {
    solarWattsA = eic_solar.GetActivePowerA()*2;
    solarWattsC = eic_solar.GetActivePowerC()*2;
  }
  else {
    solarWattsA = eic_solar.GetActivePowerA();
    solarWattsC = eic_solar.GetActivePowerC();
  }
  
  /* Is net energy positive or negative?
     We're not reading if current is pos or neg because we're only getting
     the upper 16 bit register. We are getting all 32 bits of active power though */
  if (solarWattsA < 0) solarCurrentCT1 *= -1;
  if (solarWattsC < 0) solarCurrentCT2 *= -1;

  if (LineFreq == 389) {
    totalSolarWatts = eic_solar.GetTotalActivePower()*2;
  }
  else {
    totalSolarWatts = eic_solar.GetTotalActivePower(); //all math is already done in the total register
  }
  
#endif
  /*
    DBUGS.println(" ");
    DBUGS.println("Voltage 1: " + String(voltageA) + "V");
    DBUGS.println("Voltage 2: " + String(voltageC) + "V");
    DBUGS.println("Current 1: " + String(currentCT1) + "A");
    DBUGS.println("Current 2: " + String(currentCT2) + "A");
    #ifdef SOLAR_METER
    DBUGS.println("Watts/Active Power: " + String(totalWatts) + "W");
    DBUGS.println("Solar Voltage: " + String(solarVoltageA) + "V");
    DBUGS.println("Solar Current 1: " + String(solarCurrentCT1) + "A");
    DBUGS.println("Solar Current 2: " + String(solarCurrentCT2) + "A");
    DBUGS.println("Solar Watts/AP: " + String(totalSolarWatts) + "W");
    #endif
    DBUGS.println("Power Factor: " + String(powerFactor));
    #ifdef EXPORT_METERING_VALS
    DBUGS.println("Fundamental Power: " + String(eic.GetTotalActiveFundPower()) + "W");
    DBUGS.println("Harmonic Power: " + String(eic.GetTotalActiveHarPower()) + "W");
    DBUGS.println("Reactive Power: " + String(eic.GetTotalReactivePower()) + "var");
    DBUGS.println("Apparent Power: " + String(eic.GetTotalApparentPower()) + "VA");
    DBUGS.println("Phase Angle A: " + String(eic.GetPhaseA()));
    DBUGS.println("Phase Angle C: " + String(eic.GetPhaseC()));
    #endif
    DBUGS.println("Chip Temp: " + String(temp) + "C");
    DBUGS.println("Frequency: " + String(freq) + "Hz");
    DBUGS.println(" ");

    /* For calibrating offsets - not important unless measuring small loads
       hook up CTs to meter, but not around cable
       voltage input should be connected
       average output values of the following and write to corresponding registers*/
  /*
    DBUGS.println("I1-Offset: " + String(eic.CalculateVIOffset(IrmsA, IrmsALSB)));
    DBUGS.println("I2-Offset: " + String(eic.CalculateVIOffset(IrmsC, IrmsCLSB)));
    DBUGS.println("V1-Offset: " + String(eic.CalculateVIOffset(UrmsA, UrmsALSB)));
    DBUGS.println("V2-Offset: " + String(eic.CalculateVIOffset(UrmsC, UrmsCLSB)));
    DBUGS.println("Active-Offset: " + String(eic.CalculatePowerOffset(PmeanA, PmeanALSB)));
    DBUGS.println("Reactive-Offset: " + String(eic.CalculatePowerOffset(QmeanA, QmeanALSB)));
    DBUGS.println("Funda-Offset: " + String(eic.CalculatePowerOffset(PmeanAF, PmeanAFLSB)));
  */
  /* For calibrating phase angle
     calculated phase_x angle = arccos(active / apparent)
     phi_x = round(calculated phase_x angle - actual phase_x angle) x 113.778 */
  /*
    DBUGS.println("Power A: " + String(eic.GetActivePowerA()) + "W");
    DBUGS.println("Power C: " + String(eic.GetActivePowerC()) + "W");
    DBUGS.println("Apparent A: " + String(eic.GetApparentPowerA()) + "VA");
    DBUGS.println("Apparent C: " + String(eic.GetApparentPowerC()) + "VA");
  */
  /* after calibrating phase angle, reactive should be close to 0 under a pure resistive load */
  /*
    DBUGS.println("Reactive A: " + String(eic.GetReactivePowerA()) + "var");
    DBUGS.println("Reactive C: " + String(eic.GetReactivePowerC()) + "var");
  */

#ifdef ENABLE_OLED_DISPLAY
  /* Write meter data to the display */
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("CT1:" + String(currentCT1) + "A");
  display.println("CT2:" + String(currentCT2) + "A");
  display.println("V:" + String(voltageA) + "V");
  display.println("W:" + String(totalWatts) + "W");
#ifdef SOLAR_METER
  display.println("SCT1:" + String(solarCurrentCT1) + "A");
  display.println("SCT2:" + String(solarCurrentCT2) + "A");
  display.println("SV:" + String(solarVoltageA) + "V");
  display.println("SW:" + String(totalSolarWatts) + "W");
#endif
  /*
    display.println("Freq: " + String(freq) + "Hz");
    display.println("PF: " + String(powerFactor));
    display.println("Chip Temp: " + String(temp) + "C");
  */
  display.display();
#endif

  /* Default values are passed to EmonCMS - these can be changed out for anything
     in the ATM90E32 library
  */
  strcpy(result, "");
  
  if (LineFreq == 389) {
    strcat(result, "totV:");
    dtostrf(voltageA, 2, 2, measurement);
    strcat(result, measurement);
  }
  else {
    strcat(result, "V1:");
    dtostrf(voltageA, 2, 2, measurement);
    strcat(result, measurement);

    strcat(result, ",V2:");
    dtostrf(voltageC, 2, 2, measurement);
    strcat(result, measurement);
  }

  strcat(result, ",CT1:");
  dtostrf(currentCT1, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",CT2:");
  dtostrf(currentCT2, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",totI:");
  dtostrf(totalCurrent, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",W:");
  dtostrf(totalWatts, 2, 4, measurement);
  strcat(result, measurement);

#ifdef SOLAR_METER
  strcat(result, ",SolarV:");
  dtostrf(solarVoltageA, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",SCT1:");
  dtostrf(solarCurrentCT1, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",SCT2:");
  dtostrf(solarCurrentCT2, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",totSolarI:");
  dtostrf(totalSolarCurrent, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",SolarW:");
  dtostrf(totalSolarWatts, 2, 4, measurement);
  strcat(result, measurement);
#endif
#ifdef EXPORT_METERING_VALS
  strcat(result, ",FundPow:");
  dtostrf(eic.GetTotalActiveFundPower(), 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",HarPow:");
  dtostrf(eic.GetTotalActiveHarPower(), 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",ReactPow:");
  dtostrf(eic.GetTotalReactivePower(), 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",AppPow:");
  dtostrf(eic.GetTotalApparentPower(), 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",PhaseA:");
  dtostrf(eic.GetPhaseA(), 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",PhaseC:");
  dtostrf(eic.GetPhaseC(), 2, 2, measurement);
  strcat(result, measurement);
#endif
  strcat(result, ",PF:");
  dtostrf(powerFactor, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",temp:");
  dtostrf(temp, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",freq:");
  dtostrf(freq, 2, 2, measurement);
  strcat(result, measurement);

  //DBUGS.println(result);

  input_string = result;

}
