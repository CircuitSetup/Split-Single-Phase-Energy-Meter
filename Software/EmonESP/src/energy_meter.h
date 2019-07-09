#ifndef _ENERGY_METER
#define _ENERGY_METER

//#define ENABLE_OLED_DISPLAY
//#define SOLAR_METER

/* 
 *  Uncomment to send metering values, like Fundamental, Harmonic, Reactive, Apparent Power, and Phase Angle
 */
//#define EXPORT_METERING_VALS

/* 
 * The following calibration values can be set here or in the EmonESP interface
 * EmonESP values take priority if they are set
 */
 
/* 
 * 4485 for 60 Hz (North America)
 * 389 for 50 hz (rest of the world)
 */
#define LINE_FREQ 4485

/* 
 * 0 for 10A (1x)
 * 21 for 100A (2x)
 * 42 for between 100A - 200A (4x)
 */
#define PGA_GAIN 21

/* 
 * For meter <= v1.3:
 *    42080 - 9v AC Transformer - Jameco 112336
 *    32428 - 12v AC Transformer - Jameco 167151
 * For meter > v1.4:
 *    37106 - 9v AC Transformer - Jameco 157041
 *    38302 - 9v AC Transformer - Jameco 112336
 *    29462 - 12v AC Transformer - Jameco 167151
 */
#define VOLTAGE_GAIN 37106

/*
 * 25498 - SCT-013-000 100A/50mA
 * 39473 - SCT-016 120A/40mA
 * 46539 - Magnalab 100A
 */
#define CURRENT_GAIN_CT1 39473
#define CURRENT_GAIN_CT2 39473

#ifdef SOLAR_METER
#define VOLTAGE_GAIN_SOLAR 37106
#define SOLAR_GAIN_CT1 39473
#define SOLAR_GAIN_CT2 39473
#endif

extern void energy_meter_setup();
extern void energy_meter_loop();

#endif
