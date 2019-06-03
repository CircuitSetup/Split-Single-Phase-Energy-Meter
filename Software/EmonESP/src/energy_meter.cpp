
#include "energy_meter.h"
#include "emonesp.h"
#include "emoncms.h"
#include "input.h"

// for ATM90E32 energy meter
#include <SPI.h>
#include <ATM90E32.h>


/***** CALIBRATION SETTINGS *****/
// edit in energy_meter.h
unsigned short VoltageGain = VOLTAGE_GAIN;
unsigned short CurrentGainCT1 = CURRENT_GAIN_CT1;
unsigned short CurrentGainCT2 = CURRENT_GAIN_CT2;
unsigned short LineFreq = LINE_FREQ;
unsigned short PGAGain = PGA_GAIN;


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

static const int LED_BUILTIN = 2; //for on-board LED

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 1000; //time interval in ms to send data

char result[200];
char measurement[16];

//pass CS pin and calibrations to ATM90E32 library - the 2nd (B) current channel is not used with the split phase meter 
ATM90E32 eic(CS_pin, LineFreq, PGAGain, VoltageGain, CurrentGainCT1, 0, CurrentGainCT2); 

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void energy_meter_setup() {
  /*Initialise the ATM90E32 + SPI port */
  Serial.println("Start ATM90E32");
  eic.begin();
  delay(1000);

  #if defined ESP32
  // if there is an onboard LED, set it up
  pinMode(LED_BUILTIN, OUTPUT);
  #endif

  startMillis = millis();  //initial start time

} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void energy_meter_loop()
{
  //get the current "time" (actually the number of milliseconds since the program started)
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


#if defined ESP32
    digitalWrite(LED_BUILTIN, HIGH);
#endif

  /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, totalVoltage;
  float currentCT1, currentCT2, totalCurrent;
  float totalWatts;
  float realPower, powerFactor, temp, freq;

  unsigned short sys0 = eic.GetSysStatus0(); //EMMState0
  unsigned short sys1 = eic.GetSysStatus1(); //EMMState1
  unsigned short en0 = eic.GetMeterStatus0();//EMMIntState0
  unsigned short en1 = eic.GetMeterStatus1();//EMMIntState1

  DEBUG.println("Sys Status: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
  DEBUG.println("Meter Status: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));
  delay(10);

  //if true the MCU is not getting data from the energy meter
  if (sys0 == 65535 || sys0 == 0) DEBUG.println("Error: Not receiving data from energy meter - check your connections");

  ////// VOLTAGE
  voltageA = eic.GetLineVoltageA();
  voltageC = eic.GetLineVoltageC();

  if (LineFreq == 4485)
  {
    totalVoltage = voltageA + voltageC;     //is split single phase, so only 120v per leg
  }
  else
  {
    totalVoltage = voltageA;     //voltage should be 220-240 at the AC transformer
  }

  /////// CURRENT - CT1
  currentCT1 = eic.GetLineCurrentA();

  // Is net energy positive or negative?
  unsigned short fwdEnergyA = eic.GetValueRegister(APenergyA);
  unsigned short revEnergyA = eic.GetValueRegister(ANenergyA);
  if (revEnergyA > fwdEnergyA) currentCT1 *= -1;

  /////// CURRENT - CT2
  currentCT2 = eic.GetLineCurrentC();

  // Is net energy positive or negative?
  unsigned short fwdEnergyC = eic.GetValueRegister(APenergyC);
  unsigned short revEnergyC = eic.GetValueRegister(ANenergyC);
  if (revEnergyC > fwdEnergyC) currentCT2 *= -1;


  totalCurrent = currentCT1 + currentCT2;

  /////// POWER
  realPower = eic.GetTotalActivePower();
  powerFactor = eic.GetTotalPowerFactor();
  totalWatts = (voltageA * currentCT1) + (voltageC * currentCT2);

  /////// OTHER
  temp = eic.GetTemperature();
  freq = eic.GetFrequency();

  DEBUG.println(" ");
  DEBUG.println("Voltage 1: " + String(voltageA) + "V");
  DEBUG.println("Voltage 2: " + String(voltageC) + "V");
  DEBUG.println("Current 1: " + String(currentCT1) + "A");
  DEBUG.println("Current 2: " + String(currentCT2) + "A");
  DEBUG.println("Total Watts: " + String(totalWatts) + "W");
  DEBUG.println("Active Power: " + String(realPower) + "W");
  DEBUG.println("Power Factor: " + String(powerFactor));
  DEBUG.println("Fundimental Power: " + String(eic.GetTotalActiveFundPower()) + "W");
  DEBUG.println("Harmonic Power: " + String(eic.GetTotalActiveHarPower()) + "W");
  DEBUG.println("Reactive Power: " + String(eic.GetTotalReactivePower()) + "var");
  DEBUG.println("Apparent Power: " + String(eic.GetTotalApparentPower()) + "VA");
  DEBUG.println("Phase Angle A: " + String(eic.GetPhaseA()));
  DEBUG.println("Chip Temp: " + String(temp) + "C");
  DEBUG.println("Frequency: " + String(freq) + "Hz");
  DEBUG.println(" ");

// default values are passed to EmonCMS - these can be changed out for anything
// in the ATM90E32 library 
  strcpy(result, "");

  strcat(result, "VA:");
  dtostrf(voltageA, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",VC:");
  dtostrf(voltageC, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",totV:");
  dtostrf(totalVoltage, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",IA:");
  dtostrf(currentCT1, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",IC:");
  dtostrf(currentCT2, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",totI:");
  dtostrf(totalCurrent, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",AP:");
  dtostrf(realPower, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",PF:");
  dtostrf(powerFactor, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",t:");
  dtostrf(temp, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",W:");
  dtostrf(totalWatts, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",f:");
  dtostrf(freq, 2, 2, measurement);
  strcat(result, measurement);

  DEBUG.println(result);

  input_string = result;

  #if defined ESP32
    digitalWrite(LED_BUILTIN, LOW);
  #endif
}
