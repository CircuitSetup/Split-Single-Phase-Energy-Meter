/* ATM90E32 Energy Monitor Functions

  The MIT License (MIT)

  Copyright (c) 2016 whatnick,Ryzee and Arun

  Modified to use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ATM90E32.h"

ATM90E32::ATM90E32(int pin, unsigned short lineFreq, unsigned short pgagain, unsigned short ugain, unsigned short igainA, unsigned short igainC)   // Object
{
  _cs = pin;  // SS PIN
  _lineFreq = lineFreq; //frequency of power
  _pgagain = pgagain; //PGA Gain for current channels
  _ugain = ugain; //voltage rms gain
  _igainA = igainA; //CT1
  _igainC = igainC; //CT2
}

/* CommEnergyIC - Communication Establishment */
/*
  - Defines Register Mask
  - Treats the Register and SPI Comms
  - Outputs the required value in the register
*/
unsigned short ATM90E32::CommEnergyIC(unsigned char RW, unsigned short address, unsigned short val)
{
  unsigned char* data = (unsigned char*)&val;
  unsigned char* adata = (unsigned char*)&address;
  unsigned short output;
  unsigned short address1;

  //SPI interface rate is 200 to 160k bps. It Will need to be slowed down for EnergyIC
#if !defined(ENERGIA) && !defined(ESP8266) && !defined(ESP32) && !defined(ARDUINO_ARCH_SAMD)
  SPISettings settings(200000, MSBFIRST, SPI_MODE0);
#endif

#if defined(ESP8266)
  SPISettings settings(200000, MSBFIRST, SPI_MODE2);
#endif

#if defined(ESP32)
  SPISettings settings(200000, MSBFIRST, SPI_MODE3);
#endif

#if defined(ARDUINO_ARCH_SAMD)
  SPISettings settings(200000, MSBFIRST, SPI_MODE3);
#endif

  // Switch MSB and LSB of value
  output = (val >> 8) | (val << 8);
  val = output;

  // Set R/W flag
  address |= RW << 15;

  // Swap byte address
  address1 = (address >> 8) | (address << 8);
  address = address1;

  // Transmit & Receive Data
#if !defined(ENERGIA)
  SPI.beginTransaction(settings);
#endif

  // Chip enable and wait for SPI activation
  digitalWrite(_cs, LOW);

  delayMicroseconds(10);

  // Write address byte by byte
  for (byte i = 0; i < 2; i++)
  {
    SPI.transfer (*adata);
    adata++;
  }

  // SPI.transfer16(address);
  /* Must wait 4 us for data to become valid */
  delayMicroseconds(4);

  // READ Data
  // Do for each byte in transfer
  if (RW)
  {
    for (byte i = 0; i < 2; i++)
    {
      *data = SPI.transfer (0x00);
      data++;
    }
    //val = SPI.transfer16(0x00);
  }
  else
  {
    for (byte i = 0; i < 2; i++)
    {
      SPI.transfer(*data);
      data++;
    }
    // SPI.transfer16(val);
  }

  // Chip enable and wait for transaction to end
  digitalWrite(_cs, HIGH);
  delayMicroseconds(10);
#if !defined(ENERGIA)
  SPI.endTransaction();
#endif

  output = (val >> 8) | (val << 8); // reverse MSB and LSB
  return output;

  // Use with transfer16
  // return val;
}

/* Parameters Functions*/
/*
  - Gets main electrical parameters,
  such as: Voltage, Current, Power, Energy,
  and Frequency, and Temperature

*/
// VOLTAGE
double  ATM90E32::GetLineVoltageA() {
  unsigned short voltage = CommEnergyIC(READ, UrmsA, 0xFFFF);
  return (double)voltage / 100;
}

double  ATM90E32::GetLineVoltageB() {
  unsigned short voltage = CommEnergyIC(READ, UrmsB, 0xFFFF);
  return (double)voltage / 100;
}

double  ATM90E32::GetLineVoltageC() {
  unsigned short voltage = CommEnergyIC(READ, UrmsC, 0xFFFF);
  return (double)voltage / 100;
}

// CURRENT
double ATM90E32::GetLineCurrentA() {
  unsigned short current = CommEnergyIC(READ, IrmsA, 0xFFFF);
  return (double)current / 1000;
}
double ATM90E32::GetLineCurrentB() {
  unsigned short current = CommEnergyIC(READ, IrmsB, 0xFFFF);
  return (double)current / 1000;
}
double ATM90E32::GetLineCurrentC() {
  unsigned short current = CommEnergyIC(READ, IrmsC, 0xFFFF);
  return (double)current / 1000;
}

// ACTIVE POWER
double ATM90E32::GetActivePowerA() {
  signed short apowerA = (signed short) CommEnergyIC(READ, PmeanA, 0xFFFF);
  signed short apowerAL = (signed short) CommEnergyIC(READ, PmeanALSB, 0xFFFF);
  signed short ActivePowerA = (apowerA << 8) | apowerAL; //concatenate the 2 registers to make 32 bit number
  if (ActivePowerA & 0x80000000 != 0) {
    ActivePowerA = (ActivePowerA & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)ActivePowerA * 0.00032;
}
double ATM90E32::GetActivePowerB() {
  signed short apowerB = (signed short) CommEnergyIC(READ, PmeanB, 0xFFFF);
  signed short apowerBL = (signed short) CommEnergyIC(READ, PmeanBLSB, 0xFFFF);
  signed short ActivePowerB = (apowerB << 8) | apowerBL; //concatenate the 2 registers to make 32 bit number
  if (ActivePowerB & 0x80000000 != 0) {
    ActivePowerB = (ActivePowerB & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)ActivePowerB * 0.00032;
}
double ATM90E32::GetActivePowerC() {
  signed short apowerC = (signed short) CommEnergyIC(READ, PmeanC, 0xFFFF);
  signed short apowerCL = (signed short) CommEnergyIC(READ, PmeanCLSB, 0xFFFF); 
  signed short ActivePowerC = (apowerC << 8) | apowerCL; //concatenate the 2 registers to make 32 bit number
  if (ActivePowerC & 0x80000000 != 0) {
    ActivePowerC = (ActivePowerC & 0x7FFFFFFF) * -1; //if negative get compliment
  }
  return (double)ActivePowerC * 0.00032;
}
double ATM90E32::GetTotalActivePower() {
  signed short apowerT = (signed short) CommEnergyIC(READ, PmeanT, 0xFFFF);
  signed short apowerTL = (signed short) CommEnergyIC(READ, PmeanTLSB, 0xFFFF);
  signed short TotalActivePower = (apowerT << 8) | apowerTL; //concatenate the 2 registers to make 32 bit number
  if (TotalActivePower & 0x80000000 != 0) {
    TotalActivePower = (TotalActivePower & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)TotalActivePower * 0.00032;
}
// Active Fundamental Power
double ATM90E32::GetTotalActiveFundPower() {
  signed short afundpowerT = (signed short) CommEnergyIC(READ, PmeanTF, 0xFFFF);
  signed short afundpowerTL = (signed short) CommEnergyIC(READ, PmeanTFLSB, 0xFFFF);
  signed short TotalActiveFundPower = (afundpowerT << 8) | afundpowerTL; //concatenate the 2 registers to make 32 bit number
  if (TotalActiveFundPower & 0x80000000 != 0) {
    TotalActiveFundPower = (TotalActiveFundPower & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)TotalActiveFundPower * 0.00032;
}
// Active Harmonic Power
double ATM90E32::GetTotalActiveHarPower() {
  signed short aharpowerT = (signed short) CommEnergyIC(READ, PmeanTH, 0xFFFF);
  signed short aharpowerTL = (signed short) CommEnergyIC(READ, PmeanTHLSB, 0xFFFF);
  signed short TotalActiveHarPower = (aharpowerT << 8) | aharpowerTL; //concatenate the 2 registers to make 32 bit number
  if (TotalActiveHarPower & 0x80000000 != 0) {
    TotalActiveHarPower = (TotalActiveHarPower & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)TotalActiveHarPower * 0.00032;
}


// REACTIVE POWER
double ATM90E32::GetReactivePowerA() {
  signed short rpowerA = (signed short) CommEnergyIC(READ, QmeanA, 0xFFFF);
  signed short rpowerAL = (signed short) CommEnergyIC(READ, QmeanALSB, 0xFFFF);
  signed short ReactivePowerA = (rpowerA << 8) | rpowerAL; //concatenate the 2 registers to make 32 bit number
  if (ReactivePowerA & 0x80000000 != 0) {
    ReactivePowerA = (ReactivePowerA & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)ReactivePowerA * 0.00032;
}
double ATM90E32::GetReactivePowerB() {
  signed short rpowerB = (signed short) CommEnergyIC(READ, QmeanB, 0xFFFF);
  signed short rpowerBL = (signed short) CommEnergyIC(READ, QmeanBLSB, 0xFFFF);
  signed short ReactivePowerB = (rpowerB << 8) | rpowerBL; //concatenate the 2 registers to make 32 bit number
  if (ReactivePowerB & 0x80000000 != 0) {
    ReactivePowerB = (ReactivePowerB & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)ReactivePowerB * 0.00032;
}
double ATM90E32::GetReactivePowerC() {
  signed short rpowerC = (signed short) CommEnergyIC(READ, QmeanC, 0xFFFF);
  signed short rpowerCL = (signed short) CommEnergyIC(READ, QmeanCLSB, 0xFFFF);
  signed short ReactivePowerC = (rpowerC << 8) | rpowerCL; //concatenate the 2 registers to make 32 bit number
  if (ReactivePowerC & 0x80000000 != 0) {
    ReactivePowerC = (ReactivePowerC & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)ReactivePowerC * 0.00032;
}
double ATM90E32::GetTotalReactivePower() {
  signed short rpowerT = (signed short) CommEnergyIC(READ, QmeanT, 0xFFFF);
  signed short rpowerTL = (signed short) CommEnergyIC(READ, QmeanTLSB, 0xFFFF);
  signed short TotalReactivePower = (rpowerT << 8) | rpowerTL; //concatenate the 2 registers to make 32 bit number
  if (TotalReactivePower & 0x80000000 != 0) {
    TotalReactivePower = (TotalReactivePower & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)TotalReactivePower * 0.00032;
}

// APPARENT POWER
double ATM90E32::GetApparentPowerA() {
  signed short appowerA = (signed short) CommEnergyIC(READ, SmeanA, 0xFFFF);
  signed short appowerAL = (signed short) CommEnergyIC(READ, SmeanALSB, 0xFFFF);
  signed short ApparentPowerA = (appowerA << 8) | appowerAL; //concatenate the 2 registers to make 32 bit number
  if (ApparentPowerA & 0x80000000 != 0) {
    ApparentPowerA = (ApparentPowerA & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)ApparentPowerA * 0.00032;
}
double ATM90E32::GetApparentPowerB() {
  signed short appowerB = (signed short) CommEnergyIC(READ, SmeanB, 0xFFFF);
  signed short appowerBL = (signed short) CommEnergyIC(READ, SmeanBLSB, 0xFFFF);
  signed short ApparentPowerB = (appowerB << 8) | appowerBL; //concatenate the 2 registers to make 32 bit number
  if (ApparentPowerB & 0x80000000 != 0) {
    ApparentPowerB = (ApparentPowerB & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)ApparentPowerB * 0.00032;
}
double ATM90E32::GetApparentPowerC() {
  signed short appowerC = (signed short) CommEnergyIC(READ, SmeanC, 0xFFFF);
  signed short appowerCL = (signed short) CommEnergyIC(READ, SmeanCLSB, 0xFFFF);
  signed short ApparentPowerC = (appowerC << 8) | appowerCL; //concatenate the 2 registers to make 32 bit number
  if (ApparentPowerC & 0x80000000 != 0) {
    ApparentPowerC = (ApparentPowerC & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)ApparentPowerC * 0.00032;
}
double ATM90E32::GetTotalApparentPower() {
  signed short appowerT = (signed short) CommEnergyIC(READ, SmeanT, 0xFFFF);
  signed short appowerTL = (signed short) CommEnergyIC(READ, SAmeanTLSB, 0xFFFF);
  signed short TotalApparentPower = (appowerT << 8) | appowerTL; //concatenate the 2 registers to make 32 bit number
  if (TotalApparentPower & 0x80000000 != 0) {
    TotalApparentPower = (TotalApparentPower & 0x7FFFFFFF) * -1; //if negative flip first bit and make negative again
  }
  return (double)TotalApparentPower * 0.00032;
}

// FREQUENCY
double ATM90E32::GetFrequency() {
  unsigned short freq = CommEnergyIC(READ, Freq, 0xFFFF);
  return (double)freq / 100;
}

// POWER FACTOR
double ATM90E32::GetPowerFactorA() {
  signed short pfA = (signed short) CommEnergyIC(READ, PFmeanA, 0xFFFF);
  //if negative
  if (pfA & 0x8000 != 0) {
    pfA = (pfA & 0x7FFF) * -1;
	//the register sometimes goes out of bounds for unknown reasons
	//should always be between -1 and 1
	if (pfA < -1) {
		pfA = 0; 
	}
  }
  return (double)pfA / 1000;
}
double ATM90E32::GetPowerFactorB() {
  signed short pfB = (signed short) CommEnergyIC(READ, PFmeanB, 0xFFFF);
  //if negative
  if (pfB & 0x8000 != 0) {
    pfB = (pfB & 0x7FFF) * -1;
	if (pfB < -1) {
		pfB = 0; 
	}
  }
  return (double)pfB / 1000;
}
double ATM90E32::GetPowerFactorC() {
  signed short pfC = (signed short) CommEnergyIC(READ, PFmeanC, 0xFFFF);
  //if negative
  if (pfC & 0x8000 != 0) {
    pfC = (pfC & 0x7FFF) * -1;
	if (pfC < -1) {
		pfC = 0; 
	}
  }
  return (double)pfC / 1000;
}
double ATM90E32::GetTotalPowerFactor() {
  signed short pf = (signed short) CommEnergyIC(READ, PFmeanT, 0xFFFF);
  //if negative
  if (pf & 0x8000 != 0) {
    pf = (pf & 0x7FFF) * -1;
	if (pf < -1) {
		pf = 0; 
	}
  }
  return (double)pf / 1000;
}

// MEAN PHASE ANGLE
double ATM90E32::GetPhaseA() {
  unsigned short angleA = (unsigned short) CommEnergyIC(READ, PAngleA, 0xFFFF);
  return (double)angleA / 10;
}
double ATM90E32::GetPhaseB() {
  unsigned short angleB = (unsigned short) CommEnergyIC(READ, PAngleB, 0xFFFF);
  return (double)angleB / 10;
}
double ATM90E32::GetPhaseC() {
  unsigned short angleC = (unsigned short) CommEnergyIC(READ, PAngleC, 0xFFFF);
  return (double)angleC / 10;
}

// TEMPERATURE
double ATM90E32::GetTemperature() {
  short int atemp = (short int) CommEnergyIC(READ, Temp, 0xFFFF);
  return (double)atemp;
}

/* Gets the Register Value if Desired */
// REGISTER
unsigned short ATM90E32::GetValueRegister(unsigned short registerRead) {
  return (CommEnergyIC(READ, registerRead, 0xFFFF)); //returns value register
}

// REGULAR ENERGY MEASUREMENT

// FORWARD ACTIVE ENERGY
double ATM90E32::GetImportEnergy() {
  unsigned short ienergyT = CommEnergyIC(READ, APenergyT, 0xFFFF);
  return (double)ienergyT / 100 / 3200; //returns kWh
}
  // unsigned short ienergyA = CommEnergyIC(READ, APenergyA, 0xFFFF);
  // unsigned short ienergyB = CommEnergyIC(READ, APenergyB, 0xFFFF);
  // unsigned short ienergyC = CommEnergyIC(READ, APenergyC, 0xFFFF);

// FORWARD REACTIVE ENERGY
double ATM90E32::GetImportReactiveEnergy() {
  unsigned short renergyT = CommEnergyIC(READ, RPenergyT, 0xFFFF);
  return (double)renergyT / 100 / 3200; //returns kWh
}
  // unsigned short renergyA = CommEnergyIC(READ, RPenergyA, 0xFFFF);
  // unsigned short renergyB = CommEnergyIC(READ, RPenergyB, 0xFFFF);
  // unsigned short renergyC = CommEnergyIC(READ, RPenergyC, 0xFFFF);

// APPARENT ENERGY
double ATM90E32::GetImportApparentEnergy() {
  unsigned short senergyT = CommEnergyIC(READ, SAenergyT, 0xFFFF);
  return (double)senergyT / 100 / 3200; //returns kWh
}
  // unsigned short senergyA = CommEnergyIC(READ, SenergyA, 0xFFFF);
  // unsigned short senergyB = CommEnergyIC(READ, SenergyB, 0xFFFF);
  // unsigned short senergyC = CommEnergyIC(READ, SenergyC, 0xFFFF);

// REVERSE ACTIVE ENERGY
double ATM90E32::GetExportEnergy() {
  unsigned short eenergyT = CommEnergyIC(READ, ANenergyT, 0xFFFF);
  return (double)eenergyT / 100 / 3200; //returns kWh
}
  // unsigned short eenergyA = CommEnergyIC(READ, ANenergyA, 0xFFFF);
  // unsigned short eenergyB = CommEnergyIC(READ, ANenergyB, 0xFFFF);
  // unsigned short eenergyC = CommEnergyIC(READ, ANenergyC, 0xFFFF);

// REVERSE REACTIVE ENERGY
double ATM90E32::GetExportReactiveEnergy() {
  unsigned short reenergyT = CommEnergyIC(READ, RNenergyT, 0xFFFF);
  return (double)reenergyT / 100 / 3200; //returns kWh
}
  // unsigned short reenergyA = CommEnergyIC(READ, RNenergyA, 0xFFFF);
  // unsigned short reenergyB = CommEnergyIC(READ, RNenergyB, 0xFFFF);
  // unsigned short reenergyC = CommEnergyIC(READ, RNenergyC, 0xFFFF);



/* System Status Registers */
unsigned short ATM90E32::GetSysStatus0() {
  return CommEnergyIC(READ, EMMIntState0, 0xFFFF);
}
unsigned short ATM90E32::GetSysStatus1() {
  return CommEnergyIC(READ, EMMIntState1, 0xFFFF);
}
unsigned short ATM90E32::GetMeterStatus0() {
  return CommEnergyIC(READ, EMMState0, 0xFFFF);
}
unsigned short ATM90E32::GetMeterStatus1() {
  return CommEnergyIC(READ, EMMState1, 0xFFFF);
}


/* Checksum Error Function */
bool ATM90E32::calibrationError()
{
  bool CS0, CS1, CS2, CS3;
  unsigned short systemstatus0 = GetSysStatus0();

  if (systemstatus0 & 0x4000)
  {
    CS0 = true;
  }
  else
  {
    CS0 = false;
  }

  if (systemstatus0 & 0x0100)
  {
    CS1 = true;
  }
  else
  {
    CS1 = false;
  }
  if (systemstatus0 & 0x0400)
  {
    CS2 = true;
  }
  else
  {
    CS2 = false;
  }
  if (systemstatus0 & 0x0100)
  {
    CS3 = true;
  }
  else
  {
    CS3 = false;
  }

#ifdef DEBUG_SERIAL
  Serial.print("Checksum 0: ");
  Serial.println(CS0);
  Serial.print("Checksum 1: ");
  Serial.println(CS1);
  Serial.print("Checksum 2: ");
  Serial.println(CS2);
  Serial.print("Checksum 3: ");
  Serial.println(CS3);
#endif

  if (CS0 || CS1 || CS2 || CS3) return (true);
  else return (false);

}

/* BEGIN FUNCTION */
/*
  - Define the pin to be used as Chip Select
  - Set serialFlag to true for serial debugging
  - Use SPI MODE 0 for the ATM90E32
*/
void ATM90E32::begin()
{

  // pinMode(energy_IRQ, INPUT); // (In development...)
  pinMode(_cs, OUTPUT);
  // pinMode(energy_WO, INPUT);  // (In development...)

  /* Enable SPI */
  SPI.begin();

  //SPI.setHwCs(_cs);

  Serial.println("Connecting to ATM90E32");
#if defined(ENERGIA)
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV16);
#endif

  //determine proper low and high frequency threshold
  unsigned short FreqHiThresh;
  unsigned short FreqLoThresh;

  if (_lineFreq = 4485)
  {
    //North America power frequency
    FreqHiThresh = 61 * 1000;
    FreqLoThresh = 59 * 1000;
  }
  else
  {
    FreqHiThresh = 51 * 1000;
    FreqLoThresh = 49 * 1000;
  }

  //calculation for voltage sag threshold - assumes we do not want to go under 100v for split phase and 200v otherwise
  unsigned short vSagTh;
  unsigned short sagV;
  if (_lineFreq = 4485)
  {
    sagV = 100;
  }
  else
  {
    sagV = 200;
  }

  vSagTh = (sagV * 100 * sqrt(2)) / (2 * _ugain / 32768);

  //Initialize registers
  CommEnergyIC(WRITE, SoftReset, 0x789A);   // Perform soft reset
  CommEnergyIC(WRITE, CfgRegAccEn, 0x55AA); // enable register config access
  CommEnergyIC(WRITE, MeterEn, 0x0001);   // Enable Metering


  CommEnergyIC(WRITE, SagTh, vSagTh);         // Voltage sag threshold
  CommEnergyIC(WRITE, FreqHiTh, FreqHiThresh);  // High frequency threshold - 61.00Hz
  CommEnergyIC(WRITE, FreqLoTh, FreqLoThresh);  // Lo frequency threshold - 59.00Hz
  CommEnergyIC(WRITE, EMMIntEn0, 0xB76F);   // Enable interrupts
  CommEnergyIC(WRITE, EMMIntEn1, 0xDDFD);   // Enable interrupts
  CommEnergyIC(WRITE, EMMIntState0, 0x0001);  // Clear interrupt flags
  CommEnergyIC(WRITE, EMMIntState1, 0x0001);  // Clear interrupt flags
  CommEnergyIC(WRITE, ZXConfig, 0x0A55);      // ZX2, ZX1, ZX0 pin config

  //Set metering config values (CONFIG)
  CommEnergyIC(WRITE, PLconstH, 0x0861);    // PL Constant MSB (default)
  CommEnergyIC(WRITE, PLconstL, 0xC468);    // PL Constant LSB (default)
  CommEnergyIC(WRITE, MMode0, _lineFreq);   // Mode Config (frequency set in main program, 3P3W, phase B not counted)
  CommEnergyIC(WRITE, MMode1, _pgagain);    // PGA Gain Configuration for Current Channels - 0x002A (x4) // 0x0015 (x2) // 0x0000 (1x)
  CommEnergyIC(WRITE, PStartTh, 0x0AFC);    // Active Startup Power Threshold - 50% of startup current = 0.9/0.00032 = 2812.5
  CommEnergyIC(WRITE, QStartTh, 0x0AEC);    // Reactive Startup Power Threshold
  CommEnergyIC(WRITE, SStartTh, 0x0000);    // Apparent Startup Power Threshold
  CommEnergyIC(WRITE, PPhaseTh, 0x00BC);    // Active Phase Threshold = 10% of startup current = 0.06/0.00032 = 187.5
  CommEnergyIC(WRITE, QPhaseTh, 0x0000);    // Reactive Phase Threshold
  CommEnergyIC(WRITE, SPhaseTh, 0x0000);    // Apparent  Phase Threshold

  //Set metering calibration values (CALIBRATION)
  CommEnergyIC(WRITE, PQGainA, 0x0000);     // Line calibration gain
  CommEnergyIC(WRITE, PhiA, 0x0000);        // Line calibration angle
  CommEnergyIC(WRITE, PQGainB, 0x0000);     // Line calibration gain
  CommEnergyIC(WRITE, PhiB, 0x0000);        // Line calibration angle
  CommEnergyIC(WRITE, PQGainC, 0x0000);     // Line calibration gain
  CommEnergyIC(WRITE, PhiC, 0x0000);        // Line calibration angle
  CommEnergyIC(WRITE, PoffsetA, 0x0000);    // A line active power offset
  CommEnergyIC(WRITE, QoffsetA, 0x0000);    // A line reactive power offset
  CommEnergyIC(WRITE, PoffsetB, 0x0000);    // B line active power offset
  CommEnergyIC(WRITE, QoffsetB, 0x0000);    // B line reactive power offset
  CommEnergyIC(WRITE, PoffsetC, 0x0000);    // C line active power offset
  CommEnergyIC(WRITE, QoffsetC, 0x0000);    // C line reactive power offset

  //Set metering calibration values (HARMONIC)
  CommEnergyIC(WRITE, POffsetAF, 0x0000);   // A Fund. active power offset
  CommEnergyIC(WRITE, POffsetBF, 0x0000);   // B Fund. active power offset
  CommEnergyIC(WRITE, POffsetCF, 0x0000);   // C Fund. active power offset
  CommEnergyIC(WRITE, PGainAF, 0x0000);     // A Fund. active power gain
  CommEnergyIC(WRITE, PGainBF, 0x0000);     // B Fund. active power gain
  CommEnergyIC(WRITE, PGainCF, 0x0000);     // C Fund. active power gain

  //Set measurement calibration values (ADJUST)
  CommEnergyIC(WRITE, UgainA, _ugain);      // A Voltage rms gain
  CommEnergyIC(WRITE, IgainA, _igainA);      // A line current gain
  CommEnergyIC(WRITE, UoffsetA, 0x0000);    // A Voltage offset
  CommEnergyIC(WRITE, IoffsetA, 0x0000);    // A line current offset
  CommEnergyIC(WRITE, UgainB, 0x0000);      // B Voltage rms gain
  CommEnergyIC(WRITE, IgainB, 0x0000);      // B line current gain
  CommEnergyIC(WRITE, UoffsetB, 0x0000);    // B Voltage offset
  CommEnergyIC(WRITE, IoffsetB, 0x0000);    // B line current offset
  CommEnergyIC(WRITE, UgainC, _ugain);      // C Voltage rms gain
  CommEnergyIC(WRITE, IgainC, _igainC);      // C line current gain
  CommEnergyIC(WRITE, UoffsetC, 0x0000);    // C Voltage offset
  CommEnergyIC(WRITE, IoffsetC, 0x0000);    // C line current offset

  CommEnergyIC(WRITE, CfgRegAccEn, 0x0000); // end configuration

  /*
    ERROR REGISTERS

    EMMState0
    Bit Name Description
    15 OIPhaseAST Set to 1: if there is over current on phase A
    14 OIPhaseBST Set to 1: if there is over current on phase B
    13 OIPhaseCST Set to 1: if there is over current on phase C
    12 OVPhaseAST Set to 1: if there is over voltage on phase A
    11 OVPhaseBST Set to 1: if there is over voltage on phase B
    10 OVPhaseCST Set to 1: if there is over voltage on phase C
    9 URevWnST Voltage Phase Sequence Error status
    8 IRevWnST Current Phase Sequence Error status
    7 INOv0ST When the calculated N line current is greater than the threshold set by the INWarnTh register, this bit is
    set.
    6 TQNoloadST All phase sum reactive power no-load condition status
    5 TPNoloadST All phase sum active power no-load condition status
    4 TASNoloadST All phase arithmetic sum apparent power no-load condition status
    3 CF1RevST
    Energy for CF1 Forward/Reverse status:
    0: Forward
    1: Reverse
    2 CF2RevST
    Energy for CF2 Forward/Reverse status:
    0: Forward
    1: Reverse
    1 CF3RevST
    Energy for CF3 Forward/Reverse status:
    0: Forward
    1: Reverse
    0 CF4RevST
    Energy for CF4 Forward/Reverse status:
    0: Forward
    1: Reverse


    Bit Name Description
    15 FreqHiST This bit indicates whether frequency is greater than the high threshold
    14 SagPhaseAST
    This bit indicates whether there is voltage sag on phase A
    13 SagPhaseBST
    This bit indicates whether there is voltage sag on phase B
    12 SagPhaseCST
    This bit indicates whether there is voltage sag on phase C
    11 FreqLoST This bit indicates whether frequency is lesser than the low threshold
    10 PhaseLossAST
    This bit indicates whether there is a phase loss in Phase A
    9 PhaseLossBST
    This bit indicates whether there is a phase loss in Phase B
    8 PhaseLossCST
    This bit indicates whether there is a phase loss in Phase C
    7 QERegTPST
    ReActive (Q) Energy (E) Register (Reg) of all channel total sum (T) Positive (P) Status (ST):
    0: Positive,
    1: Negative
    6 QERegAPST ReActive (Q) Energy (E) Register (Reg) of Channel (A/B/C) Positive (P) Status (ST):
    0: Positive,
    1: Negative
    5 QERegBPST
    4 QERegCPST
    3 PERegTPST
    Active (P) Energy (E) Register (Reg) of all channel total sum (T) Positive (P) Status (ST)
    0: Positive,
    1: Negative
    2 PERegAPST Active (P) Energy (E) Register (Reg) of Channel (A/B/C) Positive (P) Status (ST)
    0: Positive,
    1: Negative
    1 PERegBPST
    0 PERegCPST

    EMMIntState0
    Bit Name Description
    15 OIPhaseAIntS
    T Over current on phase A status change flag
    14 OIPhaseBIntS
    T Over current on phase B status change flag
    13 OIPhaseCInt
    ST Over current on phase C status change flag
    12 OVPhaseAInt
    ST Over Voltage on phase A status change flag
    11 OVPhaseBInt
    ST Over Voltage on phase B status change flag
    10 OVPhaseCInt
    ST Over Voltage on phase C status change flag
    9 URevWnIntST
    Voltage Phase Sequence Error status change flag
    8 IRevWnIntST Current Phase Sequence Error status change flag
    7 INOv0IntST Neutral line over current status change flag
    6 TQNoloadIntST
    All phase sum reactive power no-load condition status change flag
    5 TPNoloadIntST
    All phase sum active power no-load condition status change flag
    4 TASNoloadIntST
    All phase arithmetic sum apparent power no-load condition status change flag
    3 CF1RevIntST Energy for CF1 Forward/Reverse status change flag
    2 CF2RevIntST Energy for CF2 Forward/Reverse status change flag
    1 CF3RevIntST Energy for CF3 Forward/Reverse status change flag
    0 CF4RevIntST Energy for CF4 Forward/Reverse status change flag

    Bit Name Description
    15 FreqHiIntST FreqHiST change flag
    14 SagPhaseAIntST
    Voltage sag on phase A status change flag
    13 SagPhaseBIntST
    Voltage sag on phase B status change flag
    12 SagPhaseCIntST
    Voltage sag on phase C status change flag
    11 FreqLoIntST FreqLoST change flag
    10 PhaseLossAIntST
    Voltage PhaseLoss on phase A status change flag
    9 PhaseLossBIntST
    Voltage PhaseLoss on phase B status change flag
    8 PhaseLossCIntST
    Voltage PhaseLoss on phase C status change flag
    7 QERegTPIntST
    ReActive (Q) Energy (E) Register (Reg) of all channel total sum (T) Positive (P) status change flag (IntST)
    6 QERegAPIntST
    5 ReActive (Q) Energy (E) Register (Reg) of all channel (A/B/C) Positive (P) status change flag (IntST) QERegBPIntST
    4 QE
    RegCPIntST
    3 PERegTPIntST
    Active (P) Energy (E) Register (Reg) of all channel total sum (T) Positive (P) status change flag (IntST)
    2 PERegAPIntST
    1 Active (P) Energy(E) Register (Reg) of Channel (A/B/C) Positive (P) status change flag (IntST) PERegBPIntST
    0 PE
    RegCPIntST
  */

}