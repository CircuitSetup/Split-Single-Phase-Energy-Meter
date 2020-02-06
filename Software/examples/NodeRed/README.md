# EmonESP > MQTT > Node-Red > InfluxDB

This Node-Red flow takes MQTT data published from EmonESP (the default ESP32 program for the Split Single Phase Energy Meter), formats it, throttles it to get a sample every 10 seconds, then sends it to an InfluxDB database. 

### EmonESP Setup
- MQTT Server: Your MQTT broker address. If using Home Assistant with Mosquitto broker installed, this is the address of Home Assistant. The port defaults to 1883
- MQTT Base-topic: emon/emonesp
- MQTT Feed-name prefix: none
- Username: the username for your MQTT broker
- Password: the password for your MQTT broker

![EmonESP MQTT](/images/MQTT.png)

### InfluxDB Setup
- Go to InfluxDB Admin > Databases
- Create a new database called emoncms - set the retention policy to autogen and the duration to whatever you would like. 
- Go to Users
- Create a user named emoncms and set the permissions to all

### Node-Red Setup
- Go to Menu > Import
- Paste the contents of or select EmonESP-MQTT-NodeRed-InfluxDB.json to import
- The flow should be imported
- Double click on 'emonesp', and set the credentials for MQTT (broker)
- Click the pencil next to 'MQTT-local'. This should be the same as in the first step of EmonESP setup. Or, if you're using Home Assistant with Mosquitto, and Node-Red installed under Hass.io it can be 'localhost'. Don't forget to click on Security and set the login/password
- Double click on 'V1 to Influxdb'
- Click on the pencil next to 'emoncms' and set the credentials for the InfluxDB database. You shouldn't have to do this on all of the function nodes unless the name of the database is not 'emoncms'. 

![Node-Red Energy Meter Flow](/images/emonesp-node-red.png)

From here make sure that data is being sent to InfluxDB by going to Explore, and emoncms.autogen. You should see a graph when clicking on this, the field name, then checking value.


![InfluxDB EmonESP Data](/images/Influxdb_data.png)
