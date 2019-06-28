/* ATM90E32 Energy Monitor Functions

  The MIT License (MIT)

  Copyright (c) 2016 whatnick,Ryzee and Arun

  Modified to use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ATM90E32.h"

ATM90E32::ATM90E32(void){
}

ATM90E32::~ATM90E32() {
	// end 
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

int ATM90E32::Read32Register(signed short regh_addr, signed short regl_addr) {
  int val, val_h, val_l;
  val_h = CommEnergyIC(READ, regh_addr, 0xFFFF);
  val_l = CommEnergyIC(READ, regl_addr, 0xFFFF);
  val = CommEnergyIC(READ, regh_addr, 0xFFFF);

  val = val_h << 16;
  val |= val_l; //concatenate the 2 registers to make 1 32 bit number
  
  /*
  // returns positive value if negative
  if ((val & 0x80000000) != 0) { 
		val = (~val) + 1; //2s compliment
  }
  */
  return (val);
  
}

double ATM90E32::CalculateVIOffset(unsigned short regh_addr, unsigned short regl_addr /*, unsigned short offset_reg*/) {
//for getting the lower registers of energy and calculating the offset
//this should only be run when all inputs are disconnected
  uint32_t val, val_h, val_l;
  uint16_t offset;
  val_h = CommEnergyIC(READ, regh_addr, 0xFFFF);
  val_l = CommEnergyIC(READ, regl_addr, 0xFFFF);
  val = CommEnergyIC(READ, regh_addr, 0xFFFF);

  val = val_h << 16; //move high register up 16 bits
  val |= val_l; //concatenate the 2 registers to make 1 32 bit number
  val = val >> 7; //right shift 7 bits - lowest 7 get ignored
  val = (~val) + 1; //2s compliment
  
  offset = val; //keep lower 16 bits
  //CommEnergyIC(WRITE, offset_reg, (signed short)val);
  return uint16_t(offset);
}

double ATM90E32::CalculatePowerOffset(unsigned short regh_addr, unsigned short regl_addr /*, unsigned short offset_reg*/) {
//for getting the lower registers of energy and calculating the offset
//should only be run when CT sensors are connected to the meter,
//but not connected around wires
  uint32_t val, val_h, val_l;
  uint16_t offset;
  val_h = CommEnergyIC(READ, regh_addr, 0xFFFF);
  val_l = CommEnergyIC(READ, regl_addr, 0xFFFF);
  val = CommEnergyIC(READ, regh_addr, 0xFFFF);

  val = val_h << 16; //move high register up 16 bits
  val |= val_l; //concatenate the 2 registers to make 1 32 bit number
  val = (~val) + 1; //2s compliment
  
  offset = val; //keep lower 16 bits
  //CommEnergyIC(WRITE, offset_reg, (signed short)val);
  return uint16_t(offset);
}

double ATM90E32::CalibrateVI(unsigned short reg, unsigned short actualVal) {
//input the Voltage or Current register, and the actual value that it should be
//actualVal can be from a calibration meter or known value from a power supply
  uint16_t gain, val, m, gainReg;
	//sample the reading 
	val = CommEnergyIC(READ, reg, 0xFFFF);
	val += CommEnergyIC(READ, reg, 0xFFFF);
	val += CommEnergyIC(READ, reg, 0xFFFF);
	val += CommEnergyIC(READ, reg, 0xFFFF);
	
	//get value currently in gain register
	switch (reg) {
		case UrmsA: {
			gainReg = UgainA; }
		case UrmsB: {
			gainReg = UgainB; }
		case UrmsC: {
			gainReg = UgainC; }
		case IrmsA: {
			gainReg = IgainA; }
		case IrmsB: {
			gainReg = IgainB; }
		case IrmsC: {
			gainReg = IgainC; }
	}
		
	gain = CommEnergyIC(READ, gainReg, 0xFFFF); 
	m = actualVal;
	m = ((m * gain) / val);
	gain = m;
	
	//write new value to gain register
	CommEnergyIC(WRITE, gainReg, gain);
	
	return(gain);
}


  
/* Parameters Functions*/
/*
  - Gets main electrical parameters,
  such as: Voltage, Current, Power, Energy,
  and Frequency, and Temperature

*/
// VOLTAGE
double ATM90E32::GetLineVoltageA() {
  unsigned short voltage = CommEnergyIC(READ, UrmsA, 0xFFFF);
  return (double)voltage / 100;
}
double ATM90E32::GetLineVoltageB() {
  unsigned short voltage = CommEnergyIC(READ, UrmsB, 0xFFFF);
  return (double)voltage / 100;
}
double ATM90E32::GetLineVoltageC() {
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

double ATM90E32::GetLineCurrentN() {
  unsigned short current = CommEnergyIC(READ, IrmsN, 0xFFFF);
  return (double)current / 1000;
}

// ACTIVE POWER
double ATM90E32::GetActivePowerA() {
  int val = Read32Register(PmeanA, PmeanALSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetActivePowerB() {
  int val = Read32Register(PmeanB, PmeanBLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetActivePowerC() {
  int val = Read32Register(PmeanC, PmeanCLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetTotalActivePower() {
   int val = Read32Register(PmeanT, PmeanTLSB);
   return (double)val * 0.00032;
}

// Active Fundamental Power
double ATM90E32::GetTotalActiveFundPower() {
  int val = Read32Register(PmeanTF, PmeanTFLSB);
  return (double)val * 0.00032;
}

// Active Harmonic Power
double ATM90E32::GetTotalActiveHarPower() {
  int val = Read32Register(PmeanTH, PmeanTHLSB);
  return (double)val * 0.00032;
}


// REACTIVE POWER
double ATM90E32::GetReactivePowerA() {
  int val = Read32Register(QmeanA, QmeanALSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetReactivePowerB() {
  int val = Read32Register(QmeanB, QmeanBLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetReactivePowerC() {
  int val = Read32Register(QmeanC, QmeanCLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetTotalReactivePower() {
  int val = Read32Register(QmeanT, QmeanTLSB);
  return (double)val * 0.00032;
}

// APPARENT POWER
double ATM90E32::GetApparentPowerA() {
  int val = Read32Register(SmeanA, SmeanALSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetApparentPowerB() {
  int val = Read32Register(SmeanB, SmeanBLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetApparentPowerC() {
  int val = Read32Register(SmeanC, SmeanCLSB);
  return (double)val * 0.00032;
}
double ATM90E32::GetTotalApparentPower() {
  int val = Read32Register(SmeanT, SAmeanTLSB);
  return (double)val * 0.00032;
}

// FREQUENCY
double ATM90E32::GetFrequency() {
  unsigned short freq = CommEnergyIC(READ, Freq, 0xFFFF);
  return (double)freq / 100;
}

// POWER FACTOR
double ATM90E32::GetPowerFactorA() {
  signed short pf = (signed short) CommEnergyIC(READ, PFmeanA, 0xFFFF);
  //if negative
  if (pf & 0x8000 != 0) {
	pf = (~pf) + 1;
  }
  return (double)pf / 1000;
}
double ATM90E32::GetPowerFactorB() {
  signed short pf = (signed short) CommEnergyIC(READ, PFmeanB, 0xFFFF);
  //if negative
  if (pf & 0x8000 != 0) {
	pf = (~pf) + 1;
  }
  return (double)pf / 1000;
}
double ATM90E32::GetPowerFactorC() {
  signed short pf = (signed short) CommEnergyIC(READ, PFmeanC, 0xFFFF);
  //if negative
  if (pf & 0x8000 != 0) {
	pf = (~pf) + 1;
  }
  return (double)pf / 1000;
}
double ATM90E32::GetTotalPowerFactor() {
  signed short pf = (signed short) CommEnergyIC(READ, PFmeanT, 0xFFFF);
  //if negative
  if (pf & 0x8000 != 0) {
	pf = (~pf) + 1;
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
double ATM90E32::GetValueRegister(unsigned short registerRead) {
  return (double) CommEnergyIC(READ, registerRead, 0xFFFF); //returns value register
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

  if (CS0 || CS1 || CS2 || CS3) return (true);
  else return (false);

}

/* BEGIN FUNCTION */
/*
  - Define the pin to be used as Chip Select
  - Set serialFlag to true for serial debugging
  - Use SPI MODE 0 for the ATM90E32
*/
void ATM90E32::begin(int pin, unsigned short lineFreq, unsigned short pgagain, unsigned short ugain, unsigned short igainA, unsigned short igainB, unsigned short igainC)
{
  _cs = pin;  // SS PIN
  _lineFreq = lineFreq; //frequency of power
  _pgagain = pgagain; //PGA Gain for current channels
  _ugain = ugain; //voltage rms gain
  _igainA = igainA; //CT1
  _igainB = igainB; //CT2 - not used for single split phase meter
  _igainC = igainC; //CT2 for single split phase meter - CT3 otherwise

  pinMode(_cs, OUTPUT);

  /* Enable SPI */
  SPI.begin();

  Serial.println("Connecting to ATM90E32");
#if defined(ENERGIA)
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV16);
#endif

  //determine proper low and high frequency threshold
  unsigned short FreqHiThresh;
  unsigned short FreqLoThresh;

  if (_lineFreq == 4485 || _lineFreq == 5231)
  {
    //North America power frequency
    FreqHiThresh = 61 * 100;
    FreqLoThresh = 59 * 100;
  }
  else
  {
    FreqHiThresh = 51 * 100;
    FreqLoThresh = 49 * 100;
  }

  //calculation for voltage sag threshold - assumes we do not want to go under 90v for split phase and 190v otherwise
  unsigned short vSagTh;
  unsigned short sagV;
  if (_lineFreq == 4485 || _lineFreq == 5231)
  {
    sagV = 90;
  }
  else
  {
    sagV = 190;
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
  CommEnergyIC(WRITE, PLconstH, 0x0861);    // PL Constant MSB (default) - Meter Constant = 3200 - PL Constant = 140625000
  CommEnergyIC(WRITE, PLconstL, 0xC468);    // PL Constant LSB (default) - this is 4C68 in the application note, which is incorrect
  CommEnergyIC(WRITE, MMode0, _lineFreq);   // Mode Config (frequency set in main program)
  CommEnergyIC(WRITE, MMode1, _pgagain);    // PGA Gain Configuration for Current Channels - 0x002A (x4) // 0x0015 (x2) // 0x0000 (1x)
  CommEnergyIC(WRITE, PStartTh, 0x0AFC);    // All phase Active Startup Power Threshold - 50% of startup current = 0.09W/0.00032 = 2812.5
  CommEnergyIC(WRITE, QStartTh, 0x0000);    // All phase Reactive Startup Power Threshold
  CommEnergyIC(WRITE, SStartTh, 0x0AEC);    // All phase Apparent Startup Power Threshold
  CommEnergyIC(WRITE, PPhaseTh, 0x00BC);    // Each phase Active Phase Threshold = 10% of startup current = 0.06W/0.00032 = 187.5
  CommEnergyIC(WRITE, QPhaseTh, 0x0000);    // Each phase Reactive Phase Threshold
  CommEnergyIC(WRITE, SPhaseTh, 0x00BC);    // Each phase Apparent  Phase Threshold

  //Set metering calibration values (CALIBRATION)
  CommEnergyIC(WRITE, PQGainA, 0x0000);     // Line calibration gain
  CommEnergyIC(WRITE, PhiA, 0x0032);        // Line calibration angle - accounts for a ~2.5 degree error from 9V AC transformer
  CommEnergyIC(WRITE, PQGainB, 0x0000);     // Line calibration gain
  CommEnergyIC(WRITE, PhiB, 0x0032);        // Line calibration angle
  CommEnergyIC(WRITE, PQGainC, 0x0000);     // Line calibration gain
  CommEnergyIC(WRITE, PhiC, 0x0032);        // Line calibration angle
  CommEnergyIC(WRITE, PoffsetA, 0xFFDC);    // A line active power offset
  CommEnergyIC(WRITE, QoffsetA, 0xFFDC);    // A line reactive power offset
  CommEnergyIC(WRITE, PoffsetB, 0xFFDC);    // B line active power offset
  CommEnergyIC(WRITE, QoffsetB, 0xFFDC);    // B line reactive power offset
  CommEnergyIC(WRITE, PoffsetC, 0xFFDC);    // C line active power offset
  CommEnergyIC(WRITE, QoffsetC, 0xFFDC);    // C line reactive power offset

  //Set metering calibration values (HARMONIC)
  CommEnergyIC(WRITE, POffsetAF, 0xFFDC);   // A Fund. active power offset
  CommEnergyIC(WRITE, POffsetBF, 0xFFDC);   // B Fund. active power offset
  CommEnergyIC(WRITE, POffsetCF, 0xFFDC);   // C Fund. active power offset
  CommEnergyIC(WRITE, PGainAF, 0x0000);     // A Fund. active power gain
  CommEnergyIC(WRITE, PGainBF, 0x0000);     // B Fund. active power gain
  CommEnergyIC(WRITE, PGainCF, 0x0000);     // C Fund. active power gain

  //Set measurement calibration values (ADJUST)
  CommEnergyIC(WRITE, UgainA, _ugain);      // A Voltage rms gain
  CommEnergyIC(WRITE, IgainA, _igainA);      // A line current gain
  CommEnergyIC(WRITE, UoffsetA, 0x61A8);    // A Voltage offset
  CommEnergyIC(WRITE, IoffsetA, 0xFC60);    // A line current offset
  CommEnergyIC(WRITE, UgainB, _ugain);      // B Voltage rms gain
  CommEnergyIC(WRITE, IgainB, _igainB);      // B line current gain
  CommEnergyIC(WRITE, UoffsetB, 0x1D4C);    // B Voltage offset
  CommEnergyIC(WRITE, IoffsetB, 0xFC60);    // B line current offset
  CommEnergyIC(WRITE, UgainC, _ugain);      // C Voltage rms gain
  CommEnergyIC(WRITE, IgainC, _igainC);      // C line current gain
  CommEnergyIC(WRITE, UoffsetC, 0x1D4C);    // C Voltage offset
  CommEnergyIC(WRITE, IoffsetC, 0xFC60);    // C line current offset

  CommEnergyIC(WRITE, CfgRegAccEn, 0x0000); // end configuration
}