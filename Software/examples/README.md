## Examples ##
This example software was created by us and the community to use the CircuitSetup Split Single Phase Energy Meter in different applications.

### Sketches ###
- **ATM90E32_Basic_SPI** is a Basic Arduino sketch - this is a basic example that shows how to use the ATM90E32 library in Arduino. It simply outputs values to the serial window. The basis for this was created by [Tisham Dhar for his ATM90E26 Featherwing meter.](https://github.com/whatnick/ATM90E26_Arduino/tree/master/examples)
- **ATM90E32_MQTT** - an Arduino sketch using MQTT to send energy meter data via a Wemos D1. [Created by digiblur](https://github.com/digiblur/digiNRG_SplitPhase)
- **Home_Assistant_MQTT** - an Arduino sketch to send data to Home Assistant using MQTT. Includes the Home Assistant sensor file. Thanks to Richard Talley for supplying this.
- [**Raspberry Pi + MongoDB**](https://github.com/BitKnitting/FitHome/wiki/ElectricityMonitor) - Using a Raspberry Pi to communicate with the energy meter and send energy data to MongoDB
- [**MySQL to Home Assistant**](https://community.home-assistant.io/t/how-to-save-sensor-data-to-mysql-database/163094/6?u=circuitsetup) - Send data to a MySQL database via Arduino Ethernet, then read the database in Home Assistant. By Aaron Cake

### ATM90E32 Libraries ###
- **[CircuitPython](https://github.com/BitKnitting/CircuitSetup_CircuitPython)** - The ATM90E32 library converted to work with Adafruit's CircuitPython. Created by BitKnitting
- **[MicroPython](https://github.com/BitKnitting/CircuitSetup_micropython)** - The ATM90E32 library converted to work with MicroPython. Created by BitKnitting
- **[ESPHome](https://github.com/esphome/esphome/tree/dev)** - direct integration for ESPHome, which allows you to more easily import energy data directly into home automation systems like Home Assistant. Created by thompsa. [Some more details on setting up the meter with ESPHome](https://github.com/digiblur/digiNRG_ESPHome), by digiblur
  - **HA-ESPHome_energy_meter.yaml** - ESPHome setup config.  The default calibration values have been loaded (9V AC Transformer, 120A/40mA CTs) [Created by digiblur](https://github.com/digiblur/digiNRG_ESPHome)
  - **[ESPHome Documentation](https://esphome.io/components/sensor/atm90e32.html)** for the ATM90E32 sensor
