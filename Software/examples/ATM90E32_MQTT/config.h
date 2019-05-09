/************ WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ******************/
#define hostName "digiNRG"  

#define wifiSSID "My WIFI NAME HERE"
#define wifiPass "My PASSWORD HERE"

#define mqttHost "192.168.1.x" // mqtt IP/name
#define mqttPort 1883
#define mqttUser "username here"
#define mqttPass "password here"

#define mqttBirth "Online"
#define mqttDeath "Offline"

/******* HTTP /flash page ID/pass **********/
const char* update_username = "admin";
const char* update_password = "admin"; // change me if you want

/******* Debug **************/
#define DEBUGSERIAL
#define DEBUGTELNET  // Open a read-only telnet debug port

/******* OTA **************/
int OTAport = 8266;
#define OTApassword "OTAnrg" // change this to whatever password you want to use when you upload OTA
