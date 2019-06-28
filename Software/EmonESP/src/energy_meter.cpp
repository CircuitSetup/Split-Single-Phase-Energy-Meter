
#include "energy_meter.h"
#include "emonesp.h"
#include "emoncms.h"
#include "input.h"
#include "config.h"

// for ATM90E32 energy meter
#include <SPI.h>
#include <ATM90E32.h>


/***** CALIBRATION SETTINGS *****/
/* These values are edited in the web interface or energy_meter.h */
unsigned short VoltageGain = VOLTAGE_GAIN; //
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
const int period = 1000; //time interval in ms to send data
const int canBeNegative = 0; //set to 1 if current and power readings can be negative (like when exporting solar power)

char result[200];
char measurement[16];

ATM90E32 eic{}; //initialize the IC class

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void energy_meter_setup() {

  /*Get values from web interface and assign them if populated*/
  if(voltage_cal.toInt()>0) VoltageGain = voltage_cal.toInt();
  if(ct1_cal.toInt()>0) CurrentGainCT1 = ct1_cal.toInt();
  if(ct2_cal.toInt()>0) CurrentGainCT2 = ct2_cal.toInt();
  if(freq_cal.toInt()>0) LineFreq = freq_cal.toInt();
  if(gain_cal.toInt()>0) PGAGain = gain_cal.toInt();

  /*Initialise the ATM90E32 & Pass CS pin and calibrations to its library - 
   *the 2nd (B) current channel is not used with the split phase meter */
  Serial.println("Start ATM90E32");
  eic.begin(CS_pin, LineFreq, PGAGain, VoltageGain, CurrentGainCT1, 0, CurrentGainCT2);
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


#if defined ESP32
    digitalWrite(LED_BUILTIN, HIGH);
#endif

  /*Repeatedly fetch some values from the ATM90E32 */
  float voltageA, voltageC, totalVoltage;
  float currentCT1, currentCT2, totalCurrent;
  float totalWatts, wattsA, wattsC;
  float powerFactor, temp, freq;

  unsigned short sys0 = eic.GetSysStatus0(); //EMMState0
  unsigned short sys1 = eic.GetSysStatus1(); //EMMState1
  unsigned short en0 = eic.GetMeterStatus0();//EMMIntState0
  unsigned short en1 = eic.GetMeterStatus1();//EMMIntState1

  DEBUG.println("Sys Status: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
  DEBUG.println("Meter Status: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));
  delay(10);

  /*if true the MCU is not getting data from the energy meter */
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

  /////// CURRENT & POWER
  currentCT1 = eic.GetLineCurrentA();
  currentCT2 = eic.GetLineCurrentC();
  
  wattsA = eic.GetActivePowerA();
  wattsC = eic.GetActivePowerC();
  
  if (canBeNegative == 1) {
    /* Is net energy positive or negative?
     * We're not reading if current is pos or neg because we're only getting
     * the upper 16 bit register. We are getting all 32 bits of active power though */
    if (wattsA < 0) currentCT1 *= -1; 
    if (wattsC < 0) currentCT2 *= -1;
    totalWatts = eic.GetTotalActivePower(); //all math is already done in the total register
  }
  else { 
    /* If single split phase & 1 voltage reading, phase will be offset between the 2 phases
     * making one power reading negative. This will correct for that negative reading. */
    if (wattsA < 0) wattsA *= -1;
    if (wattsC < 0) wattsC *= -1;
    totalWatts = wattsA + wattsC;
  }
  
  totalCurrent = currentCT1 + currentCT2;
  powerFactor = eic.GetTotalPowerFactor();

  /////// OTHER
  temp = eic.GetTemperature();
  freq = eic.GetFrequency();

  DEBUG.println(" ");
  DEBUG.println("Voltage 1: " + String(voltageA) + "V");
  DEBUG.println("Voltage 2: " + String(voltageC) + "V");
  DEBUG.println("Current 1: " + String(currentCT1) + "A");
  DEBUG.println("Current 2: " + String(currentCT2) + "A");
  DEBUG.println("Active Power: " + String(totalWatts) + "W");
  DEBUG.println("Power Factor: " + String(powerFactor));
  DEBUG.println("Fundamental Power: " + String(eic.GetTotalActiveFundPower()) + "W");
  DEBUG.println("Harmonic Power: " + String(eic.GetTotalActiveHarPower()) + "W");
  DEBUG.println("Reactive Power: " + String(eic.GetTotalReactivePower()) + "var");
  DEBUG.println("Apparent Power: " + String(eic.GetTotalApparentPower()) + "VA");
  DEBUG.println("Phase Angle A: " + String(eic.GetPhaseA()));
  DEBUG.println("Chip Temp: " + String(temp) + "C");
  DEBUG.println("Frequency: " + String(freq) + "Hz");
  DEBUG.println(" ");
  
  /* For calibrating offsets - not important unless measuring small loads
   * hook up CTs to meter, but not around cable
   * voltage input should be connected
   * average output values of the following and write to corresponding registers*/
  /*
  DEBUG.println("I1-Offset: " + String(eic.CalculateVIOffset(IrmsA, IrmsALSB)));
  DEBUG.println("I2-Offset: " + String(eic.CalculateVIOffset(IrmsC, IrmsCLSB)));
  DEBUG.println("V1-Offset: " + String(eic.CalculateVIOffset(UrmsA, UrmsALSB)));
  DEBUG.println("V2-Offset: " + String(eic.CalculateVIOffset(UrmsC, UrmsCLSB)));
  DEBUG.println("Active-Offset: " + String(eic.CalculatePowerOffset(PmeanA, PmeanALSB)));
  DEBUG.println("Reactive-Offset: " + String(eic.CalculatePowerOffset(QmeanA, QmeanALSB)));
  DEBUG.println("Funda-Offset: " + String(eic.CalculatePowerOffset(PmeanAF, PmeanAFLSB)));
  */
  /* For calibrating phase angle
   * calculated phase_x angle = arccos(active / apparent) 
   * phi_x = round(calculated phase_x angle - actual phase_x angle) x 113.778 */
  /*
  DEBUG.println("Power A: " + String(eic.GetActivePowerA()) + "W");
  DEBUG.println("Power C: " + String(eic.GetActivePowerC()) + "W");
  DEBUG.println("Apparent A: " + String(eic.GetApparentPowerA()) + "VA");
  DEBUG.println("Apparent C: " + String(eic.GetApparentPowerC()) + "VA"); 
  */
  /* after calibrating phase angle, reactive should be close to 0 under a pure resistive load */
  /*
  DEBUG.println("Reactive A: " + String(eic.GetReactivePowerA()) + "var");
  DEBUG.println("Reactive C: " + String(eic.GetReactivePowerC()) + "var");
  */

// default values are passed to EmonCMS - these can be changed out for anything
// in the ATM90E32 library 
  strcpy(result, "");

  strcat(result, "V1:");
  dtostrf(voltageA, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",V2:");
  dtostrf(voltageC, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",totV:");
  dtostrf(totalVoltage, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",CT1:");
  dtostrf(currentCT1, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",CT2:");
  dtostrf(currentCT2, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",totI:");
  dtostrf(totalCurrent, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",PF:");
  dtostrf(powerFactor, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",temp:");
  dtostrf(temp, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",W:");
  dtostrf(totalWatts, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",freq:");
  dtostrf(freq, 2, 2, measurement);
  strcat(result, measurement);

  //DEBUG.println(result);

  input_string = result;

  #if defined ESP32
    digitalWrite(LED_BUILTIN, LOW);
  #endif
}
