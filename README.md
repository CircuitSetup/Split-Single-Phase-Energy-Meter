# CitcuitSetup Split Single Phase Energy Meter

The CircuitSetup ATM90E32 Split Single Phase Energy Meter can monitor the energy usage in your entire home in real time. It can easily be hooked up to an ESP8266 or ESP32 to wirelessly transmit energy usage data into a program like EmonCMS. It can also be used to monitor solar power generation to keep track of how much power you are making.
<h4><strong>With the Split Single Phase Energy Meter you can:</strong></h4>
<ul>
 	<li><strong>Save Money!</strong>
<ul>
 	<li>See exactly how much money is being spent on energy in real time</li>
 	<li>Find appliances that are using too much electricity</li>
 	<li>Calculate energy usage for a single room to divide an energy bill fairly among roommates</li>
</ul>
</li>
 	<li><strong>View &amp; Gather Energy Data</strong>
<ul>
 	<li>View the energy usage of your entire home</li>
 	<li>Track solar power generation (2 units required)</li>
 	<li>Calculate how much it costs to charge your electric vehicle</li>
 	<li>Remote energy monitoring for vacation or rental properties</li>
 	<li>Review and graph historical energy data</li>
</ul>
</li>
 	<li><strong>Be Informed!</strong>
<ul>
 	<li>Independent of your power utility's meter</li>
 	<li>Set up alerts for over or under usage</li>
 	<li>Prevent surprises on energy bills</li>
 	<li>View usage data in the EmonCMS Android or iOS apps</li>
 	<li>Automate notifications with your home automation system like "send my phone a message when the dryer is done", or even "if I leave the house, and the oven is on, send me an alert" (programming required)</li>
</ul>
</li>
 	<li><strong>Spend less on energy monitoring hardware!</strong>
<ul>
 	<li>Affordable but very accurate</li>
 	<li>Save hundreds over popular monitoring systems</li>
</ul>
</li>
</ul>
<h4><strong>Features:</strong></h4>
<ul>
 	<li>Uses the <a href="https://www.microchip.com/wwwproducts/en/atm90e32as">Microchip ATM90E32AS</a></li>
 	<li>Samples 2 current channels &amp; 1 voltage channel (expandable to 2 voltage)</li>
 	<li>Calculates:
<ul>
 	<li>Active Power</li>
 	<li>Reactive Power</li>
 	<li>Apparent Power</li>
 	<li>Power Factor</li>
 	<li>Frequency</li>
 	<li>Temperature</li>
</ul>
</li>
 	<li>Uses standard current transformer clamps to sample current</li>
 	<li>Includes built-in buck converter to power ESP8266, or ESP32</li>
 	<li>2 IRQ interrupts, and 1 Warning output</li>
 	<li>Energy pulse output (pulses correspond to 4 LEDs)</li>
 	<li>Zero crossing output</li>
 	<li>SPI Interface</li>
 	<li>Measurement Error: 0.1%</li>
 	<li>Dynamic Range: 6000:1</li>
 	<li>Gain Selection: Up to 4x</li>
 	<li>Voltage Reference Drift Typical (ppm/°C): 6</li>
 	<li>ADC Resolution (bits): 16</li>
 	<li>Compact size at only 40x50mm</li>
</ul>
<div class="table__row">

&nbsp;

</div>
<h4><strong>What you'll need:</strong></h4>
<ul>
 	<li>Current Transformers:  <a href="https://amzn.to/2E0KVvo">SCT-013-000 100A/50mA</a> (select the version of the board with phono connectors) or <a href="https://amzn.to/2IF8xnY">Magnelab SCT-0750-100</a> (must sever burden resistor connection on the back of the board since they have a built in burden resistor). Others can also be used as long as they're rated for the amount of power that you are wanting to measure, and have a current output no more than 600mA.</li>
 	<li>AC Transformer: <a href="https://amzn.to/2XcWJjI">Jameco Reliapro 9v</a></li>
 	<li>An <a href="https://amzn.to/2pCtTtz">ESP32</a> (ESP8266 or anything else that has an SPI interface &amp; recommended wifi)</li>
 	<li>Jumper wires with Dupont connectors, or perf board to connect the two boards.</li>
 	<li>The software located here to load onto your controller
 	<li><a href="https://emoncms.org/site/home">EmonCMS </a>(shown in pictures), ThingSpeak or similar
