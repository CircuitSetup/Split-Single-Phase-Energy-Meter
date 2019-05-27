#ifndef _ENERGY_METER
#define _ENERGY_METER

// 4485 for 60 Hz (North America)
// 389 for 50 hz (rest of the world)
#define LINE_FREQ 4485

// 0 for 10A (1x)
// 21 for 100A (2x)
// 42 for between 100A - 200A (4x)
#define PGA_GAIN 21

// 41820 - 9v AC transformer
// 32428 - 12v AC Transformer
#define VOLTAGE_GAIN 41820

// 25498 - SCT-013-000 100A/50mA
// 46539 - Magnalab 100A/200A
#define CURRENT_GAIN_CT1 46539

// 25498 - SCT-013-000 100A/50mA
// 46539 - Magnalab 100A/200A
#define CURRENT_GAIN_CT2 25498

extern void energy_meter_setup();
extern void energy_meter_loop();

#endif
