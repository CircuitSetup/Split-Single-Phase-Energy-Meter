# CircuitSetup Split Single Phase Energy Meter

The CircuitSetup ATM90E32 Split Single Phase Energy Meter can monitor the energy usage in your entire home in real time. It can easily be hooked up to an ESP8266 or ESP32 to wirelessly transmit energy usage data into a program like EmonCMS. It can also be used to monitor solar power generation to keep track of how much power you are making.

![CitcuitSetup Split Single Phase Energy Meter](images/energy_meter_small.jpg)

#### With the Split Single Phase Energy Meter you can:

*   **Save Money!**
    *   See exactly how much money you're spending on energy in real time
    *   Find appliances that are using too much electricity
    *   Calculate energy usage for a single room to divide an energy bill fairly among roommates
*   **View & Gather Energy Data**
    *   View the energy usage of your entire home
    *   Track solar power generation (two units required)
    *   Calculate how much it costs to charge your electric vehicle
    *   Remote energy monitoring for vacation or rental properties
    *   Review and graph historical energy data
*   **Be Informed!**
    *   Independent of your power utility's meter
    *   Set up alerts for over or under usage
    *   Prevent surprises on energy bills
    *   View usage data in the EmonCMS Android or iOS apps
    *   Automate notifications with your home automation system like "send my phone a message when the dryer is done", or even "if I leave the house, and the oven is on, send me an alert" (programming required)
*   **Spend less on energy monitoring hardware!**
    *   Affordable but very accurate
    *   Save hundreds over popular monitoring systems

The Energy Meter kit available on CrowdSupply:
![Energy Meter Kit](/images/spiltsinglephase-angle-open_png_project-body.jpg?raw=true)


## Contents

1.  [Software Setup](#software-setup)
2.  [Hardware Setup](#hardware-setup)
3.  [Calibration](#calibration)

## Features:

* **IC: MicroChip ATM90E32**
* **Connectivity**
     * SPI Interface to connect to any Arduino compatible MCU
     * Two IRQ interrupts, and one Warning output
     * Energy pulse output (pulses correspond to four LEDs)
     * Zero crossing output
* **Real Time Data Sampling**
     * Two current channels - up to 200A per channel
     * One voltage channel (expandable to two)
     * Measurement Error: 1%
     * Dynamic Range: 6000:1
     * Gain Selection: Up to 4x
     * Voltage Reference Drift Typical (ppm/°C): 6
     * ADC Resolution (bits): 16
* **Calculates**
     * Active Power
     * Reactive Power
     * Apparent Power
     * Power Factor
     * Frequency
     * Temperature
* **Other Features**
     * Can use more than one at a time to measure as many circuits as you want, including solar power generation
     * Uses standard current transformer clamps to sample current
     * Includes built-in 500mA 3v3 buck converter to power MCU board 
     * Compact size at only 40 mm x 50 mm

## What you'll need:

* Current Transformers for wire:
   * **13mm and less**, the [SCT-013-000 100A/50mA](https://circuitsetup.us/index.php/product/100a-50ma-current-transformer-yhdc-sct-013/) has a 13mm opening for wire.
   * **13.5mm - 16mm - 4/0 AWG** (usually aluminum, 200A service) not thicker than 16mm with insulation, we recommend the [YHDC SCT-016 120A/40mA](https://circuitsetup.us/index.php/product/120a-40ma-current-transformer-yhdc-sct-016-with-3-5mm-jack-16mm-opening/). The ones available on CircuitSetup.us, and included with the energy meter kit have 3.5mm plugs. 
   * **Greater than 16mm**, the [Magnelab SCT-0750-100](https://amzn.to/2IF8xnY) (must sever burden resistor connection on the back of the board since they have a built in burden resistor). These have wire leads and will need an adapter or screw connectors soldered to the energy meter board.
   * Other CTs can also be used as long as they're rated for the amount of power that you are wanting to measure, and have a current output no more than 600mA. For safety, they MUST have a built in zener diode or burden resistor. 
* AC Transformer: [Jameco Reliapro 9v](https://amzn.to/2XcWJjI)
* An [ESP32](https://amzn.to/2pCtTtz), ESP8266, LoRa, or anything else that has an SPI interface.
* Jumper wires with Dupont connectors, or perf board to connect the two boards. The energy meter kit comes with a PCB adapter for an ESP32.
* The software located here to load onto your controller
* [EmonCMS](https://emoncms.org/site/home), ThingSpeak, InfluxDB/Graphina, or similar


## Software Setup

If you purchased an energy meter kit EmonESP is pre-loaded onto the included ESP32. You can skip this section and go to [Setting up EmonCMS](#setting-up-emoncms)

1.  Clone this repository in Github desktop or [download all the files and extract them to a folder](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/archive/master.zip)
2.  Place the ATM90E32 folder in your Arduino libraries folder. This is usually under Documents > Arduino > libraries
3.  We highly recommend using [EmonCMS.](https://emoncms.org/site/home) - EmonESP helps to connect and send data directly to EmonCMS
4.  Open **EmonESP > src > src.ino** - you will see a number of files open, but you'll only need to worry about src.ino
5.  Make sure the **CS_pin** is set to the pin that you are using on your controller board - the defaults are listed in src.ino and in the [hardware section here.](#hardware-setup)
6.  Upload the src.ino to your ESP (If you get any errors at this point, like a missing library, check the [Troubleshooting section on the EmonESP readme.](Software/EmonESP#troubleshooting-upload))
   If you are using an ESP32, [make sure you are using the latest software from the Espressif repository.](https://github.com/espressif/arduino-esp32)
7.  Upload files to the ESP in the data directory via SPIFFS - [see details on how to do this here](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/tree/master/Software/EmonESP#2-install-esp-filesystem-file-uploader)
8.  Follow the directions to configure the Access Point in the [EmonESP directions](/Software/EmonESP#first-setup)

### Setting up EmonCMS 

There are a few options for doing this:
- You can use the [EmonCMS.org service](https://emoncms.org/site/home), which costs roughly $15 a year with the data that we send from the energy meter (you don't _have_ to send all of the data)
- [Install on a computer within your network](https://github.com/emoncms/emoncms). To do this, you will need to have [apache/php/mysql installed](https://www.znetlive.com/blog/how-to-install-apache-php-and-mysql-on-windows-10-machine/)) [This can also be done with a Raspberry Pi.](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/readme.md)
- [Install on a remote web server](https://github.com/emoncms/emoncms). There are some very cheap ways this can be done if you're familar with setting up web applications.

If you install EmonCMS on a remote web server, or if your home network has a public facing port, this will make it possible to see data on the EmonCMS app ([Android](https://play.google.com/store/apps/details?id=org.emoncms.myapps) or [iOS](https://itunes.apple.com/us/app/emoncms/id1169483587?ls=1&mt=8)) when your phone is outside of your network.

For all but the EmonCMS.org service, (currently for EmonCMS.org these feeds and inputs have to be setup manually) you can automatically setup the energy meter device in EmonCMS:
1. [Install the device plugin](https://github.com/emoncms/device) 
2. [Upload this file to the Modules > device > data > CircuitSetup folder.](/Software/EmonCMS/circuitsetup_split-phase.json)
3. Once the folder is created and json file is uploaded, go to Setup (top left) > Device Setup > New Device (lower right)
4. Click on CircuitSetup in the left menu
   You will see this: 
   ![EmonCMS Device](/images/emoncms_device.PNG?raw=true)
5. Fill in the Name and Location and click save. 
6. You will then see the fields and inputs - click Initialize:
   ![EmonCMS Device2](/images/emoncms_device2.PNG?raw=true)
7. You should now see this under Feeds:
   ![EmonCMS Feeds](/images/emoncms_device_feeds.PNG?raw=true)
8. And this under Inputs:
   ![EmonCMS Inputs](/images/emoncms_device_inputs2.PNG?raw=true)

### Other software options
If you would like to use something other than EmonCMS, you can do that too! Make sure the ATM90E32 library is included in the sketch. See the [examples folder](/Software/examples) for examples of how things could be done using JSON or MQTT. Users have already setup feeds to Home Assistant, Influxdb, and Graphina. 

Support for this energy meter, and the ATM90E32 is not included in (the dev branch of) [ESPHome.](https://github.com/esphome/esphome/tree/dev)

## Hardware Setup
If you purchased an energy meter kit you can skip this section and go to [Installing the Energy Meter](#installing-the-energy-meter)

### Connect your controller to the energy meter
Connect the pins on the Energy Monitor to your MCU. If you have the adapter board, everything should already be connected properly. The following are the default for each, but they can be changed in software if you are using these pins for something else. 

For the ESP32:
*   5 - CS
*   18 - CLK
*   19 - MISO
*   23 - MOSI

For the ESP8266:
*   D8/16 - CS
*   D5/14 - CLK
*   D6/12 - MISO
*   D7/13 - MOSI

Don't forget to hook up the 3V3 and GND pins! 

**The energy meter is capable of supplying up to 500mA of 3.3v power to your controller**, so no other external power source should be needed. Some ESP32 dev boards may use more than 500mA when trying to initially connect to Wifi. If this is the case you may not be able to connect to wifi. If this happens, we recommend using another power source for the ESP32 - either a 5v DC adapter or a USB phone charger that outputs at least 500mA. It is **not** recommended to leave USB power plugged into an ESP at the same time as the energy meter's power 3V3 output. This can damage components. 

Other pins on your controller can be connected to the WARN and IRQ outputs, but they are not yet implemented in the default software. 

### Using more than one meter
More than one meter can be hooked up to a single MCU to monitor a solar grid, for example. 

To do this:
- Connect all CLK, MISO, and MOSI pins together
- Connect the GND and 3V3 from one meter to the MCU
- Connect the CS pin of one energy meter to an open GPIO on your MCU, and the CS pin on the second energy meter to another open GPIO on your MCU. These pins have to be set in the software. See the [examples folder](/Software/examples) for the example with more than one energy meter.
- If you would like to monitor voltage from two sources, you will need two AC transformers. If only one voltage, you can split the output of 1 AC transformer using a 2.5mm DC jack Y-cable.

## Installing the Energy Meter
### Warning
**High voltage AC power is VERY dangerious! If you are not comfortable working around AC voltage, we strongly encourage you to hire a qualified electrician.**

### **Disclaimer**
**The Split-Single Phase Energy Meter should be installed by a qualified professional, and in compliance with all local electrical codes that apply. CircuitSetup, and its parent company Sugarman Studios, LLC, can not be held liable for damages or injury incured by incorrectly installing the Split-Single Phase Energy Meter.**

1. Decide where to mount the energy meter. We recommend installing the box outside of your panel for a better wifi signal.  **The current transformer wires will need to be passed through a grommet in the side of the panel** It is against NEC code (US) to not route wires going in or out of an electrical panel through a grommet. 
2. The AC Transformer should be plugged into an outlet close to the panel. If you do not have one close, it is recommened that you have one installed by a licensed electrician. This isn't absolutely needed, but it will give you more accurate readings. 
3. If you wish to read the **voltage from both sides of your panel**:
   - Hook up a second AC transformer to a single pole breaker, or a second outlet, that is wired to the opposite phase of the first AC transformer. So if the breaker for the first outlet is on the left of your panel, you will need to wire the second AC transformer or outlet to a breaker on the right of the panel (Split phase US breaker panels)
   - Sever jumper JP3 on the back of the energy meter **before hooking up the second AC transformer.** 
   - Solder 2 pin headers to the right of the main AC power plug labeled GND and VC+. 
   - Hook up the second AC transformer to the GND and AC+ pins
   - Modify software to accomodate for this second voltage reading



### Connect Current Transformers to the energy meter
Before connecting the current transformers to your mains wires, plug them into the energy meter.

If your current transformers (CTs) have 3.5mm phono connectors, you hopefully have the version of the Energy Meter with these connections (v1.3 has footprints for both). If you have the screw connectors, the phono connectors will have to be cut off. There should only be two wires regardless. For the screw connector version, be careful to connect the positive to the correct terminal. If these are reversed, things will not be damaged, but the reading will read negative.

If your current transformers have a built in burden resistor, be sure to sever the jumpers on the back of the board to disable the 12ohm burden resistor. Alternatively, if you are reading smaller loads and would like more accurate readings, you can insert your own higher value burden resister across the positive and negative screw terminals.

If you purchased a kit with the black SCT016 current transformers, or have the blue SCT-013-000, these do not have a built in burden resistor, but have a TVS diode instead. 

### Connect Current Transformers to your mains
1. Note the direction of the arrows on the top of the current transformers. They should point in the direction of the current flowing into your house.
2. Clip the current transformers around the two large main wires, usually at the top of the breaker box. **DO NOT TOUCH BARE METAL ON THESE WIRES**. There is one current transformer for each phase. 
3. Make sure the current transformers are not in the way of anything else and are snapped closed around the wires. 

![SCT016 Install](/images/current_transformer_SCT016-install2.jpg)

#### Solar Current Transformers ####
Connect current transformers for solar energy generation at the connection to your main panel. Make sure that the direction of the arrows is pointing to your electrical panel, since energy is flowing into your panel. 

Energy generated by solar, but not used by your house will register on the mains CTs as negative, since energy is flowing back to the grid. 

## Calibration

If you purchased a kit that came with current transformers **and** an AC Transformer, you should not have to calibrate anything unless you want to. If you are providing your own AC Transformer, you will need to calibrate the voltage.

The default configuration of the Energy Meter software is set to use the SCT-016 120A/40mA current transformers, and the [Jameco Reliapro 9v AC transformer](https://amzn.to/2XcWJjI). There are also values for 100A Magnalab, SCT-013 100A 50mV current tranaformers, and the 12v version of the AC transformer located in the **energy_meter.h** file, and in the EmonESP web interface. Simply change the values under CALIBRATION SETTINGS if you are using a 12v AC Transformer or the Magnalab current transformers.  **If you are using any of these you likely will not need to calibrate, but if you want to be sure your readings are the most accurate then calibration is recommended.** 

Alternatively, if you have equipment that can read active and reactive energy pulse outputs, CT1-CT4 pins can be used for this. It is recommended that these connections are opto-isolated to prevent interference. 

### For calibration you will need:
1.  A multi-meter, or to make it easier and safer, a [kill-a-watt](https://amzn.to/2TXT7jx) or similar. A clamp meter will also work if you would like to measure mains power directly.
2.  A hair dryer, soldering iron, electric heater, or anything else that uses a large amount of resistive current.
3.  A modified power cable that allows you to put a current transformer around only the hot (usually black) wire.

### Setup
1.  At this point all wires should be connected between your ESP and the Energy Monitor.
2.  Connect the Energy Monitor to the AC Transformer and plug it in - the ESP and Energy Meter should both have power. If either do not, check your connections.

#### With the Arduino IDE
1.  Open the Arduino IDE
2.  Connect your ESP to your computer via USB cable
3.  Select the COM port under Tools > Port
4.  Go to Tools > Serial Monitor
5.  Values should be scrolling by. If you do not see anything in the serial window, make sure the correct COM port is selected for your ESP in the Arduino IDE.

#### With EmonESP
1.  The first time the ESP32 or ESP8266 is started with EmonESP it will not be connected to local router, and will go into AP mode. You can connect to the interface via a cell phone or other wireless device to access the interface via a web browser at 192.168.4.1 or emonesp.local. For more details on this see [EmonESP WiFi Connection](/Software/EmonESP#1-wifi-connection)
2.  If the ESP is already connected to a network, you can access the interface with the IP address that your router assigned to it.
3.  Go to the Calibration section:

![EmonESP Cal](/images/emonesp_calibration.PNG)

### Voltage Procedure 

1.  In the Serial Monitor window or in EmonESP under 4. Latest Data, view the value for Voltage - take note of this (if you are getting a value above 65k, something is not hooked up or working correctly)
2.  Take a reading of the actual voltage from an outlet in your house.  For the Kill-a-watt, just plug it in, and select voltage. Compare the values.
3.  Adjust the value for VoltageGain in energy_meter.h or EmonESP by calculating:

<pre>New VoltageGain = (your voltage reading / energy monitor voltage reading) * VOLTAGE_GAIN</pre>

VOLTAGE_GAIN is the value currently set for the voltage gain parameter in energy_meter.h.

Test again after adjusting the value and re-uploading the sketch to your ESP. If it is still off, do the procedure again, but replace VOLTAGE_GAIN with the last value used. 

If you are not located in the US and have 50hz power, change the Frequency value to 389

### Current Procedure
For calibrating **CurrentGainCT1 & CurrentGainCT2**:

1.  In the Serial Monitor window or in EmonESP under 4. Latest Data, view the value for Current 
2.  If the value is negative, you are either exporting power (solar), or the current transformer is backwords.
3.  Compare what you are seeing for current from the Energy Monitor to the reading on the Kill-a-watt
4.  Adjust the value for CurrentGainCT1 or CurrentGainCT2 in energy_meter.h or EmonESP by calculating:

<pre>New CurrentGainCT1 or CurrentGainCT2 = (your current reading / energy monitor current reading) * CURRENT_GAIN_CT#</pre>

CURRENT_GAIN_CT# is the value currently set for the current gain parameter in energy_meter.h.

Test again after adjusting the value and re-uploading the sketch to your ESP. If it is still off, do this again, but replace the CURRENT_GAIN_CT# with the last value used. It is possible that the two identical current sensors will have different CurrentGain numbers due to variances in manufacturing, but it shouldn't be drastic. Note that the positioning of the CT sensor on the hot wire can have an effect on the current reading. 

For more details, see the Calibration Procedure in the [Microchip Application notes.](http://ww1.microchip.com/downloads/en/AppNotes/Atmel-46103-SE-M90E32AS-ApplicationNote.pdf)


### Licenses
Hardware license: CERN v 1.2
Documentation license: CC BY 4.0
