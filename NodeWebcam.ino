#include <FS.h> 

#include <ArduinoJson.h>  
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>        
#include <ESP8266httpUpdate.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h> 
//#include <WiFiClient.h>
#include <WiFiClientSecure.h> 
#include <PubSubClient.h>

#include <Ticker.h>
#include <Bounce2.h>

#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>

#include "memorysaver.h" // config file in Arducam library
#include "config.h"
#if !(defined ESP8266 )
#error Please select the ArduCAM ESP8266 UNO board in the Tools/Board
#endif

//WiFiClientSecure httpClient;
WiFiClientSecure MqttWifiClient;
ESP8266WiFiMulti WiFiMulti;
PubSubClient MqttClient(MqttWifiClient);
ArduCAM myCAM(OV2640, CS);
Bounce debouncer = Bounce();
Ticker ticker;

void tick() {
  int state = digitalRead(BUILTIN_LED); 
  digitalWrite(BUILTIN_LED, !state); 
}

void setPins(){
    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, HIGH);
    pinMode(OTA_BUTTON_PIN, INPUT_PULLUP);
    debouncer.attach(OTA_BUTTON_PIN);
    debouncer.interval(3000);
    Serial.println(F("Pins set"));
}

void checkButton() {
  debouncer.update();
  int value = debouncer.read();
  if (value == LOW) {
      Serial.println(F("Long push detected, ask for config"));
      manualConfig = true;
      configManager();
  }
}

void getDeviceId() {
  char msgBuffer[8];         
  char *espChipId;
  float chipId = ESP.getChipId();
  espChipId = dtostrf(chipId, 8, 0, msgBuffer);
  strcpy(deviceId,devicePrefix); 
  strcat(deviceId,espChipId);
}

void saveConfigCallback () {
  Serial.println(F("Should save config"));
  shouldSaveConfig = true;
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
 // delay(1000);
  Serial.println(F("Entered config mode"));
  Serial.println(WiFi.softAPIP());
//  while (myWiFiManager->startConfigPortal(deviceId, devicePass)) {
//  checkButton2();
//      Serial.println(F("Hello"));
//  }
//  pinMode(MY_INCLUSION_MODE_BUTTON_PIN, INPUT_PULLUP);
//  if (digitalRead(MY_INCLUSION_MODE_BUTTON_PIN) == LOW) {

//    Serial.println(F("Push detected, ask for reset"));
//    //wifiConfigTime = 10;
//    setup();
//  }  
//  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void before() {
  Serial.println();
  Serial.println(F("mounting FS..."));
  if (SPIFFS.begin()) {
    Serial.println(F("mounted file system"));
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println(F("reading config file"));
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println(F("opened config file"));
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_client, json["mqtt_client"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_password, json["mqtt_password"]);
          strcpy(http_server, json["http_server"]);
          strcpy(http_port, json["http_port"]);
          strcpy(mqtt_topic_in,mqtt_client); 
          strcat(mqtt_topic_in,in); 
          strcpy(mqtt_topic_out,mqtt_client);
          strcat(mqtt_topic_out,out);
          strcpy(post_destination,post_prefix);
          strcat(post_destination,mqtt_client); 
          mqttServer = mqtt_server;
          mqttPort = atoi(mqtt_port);
          mqttClient = mqtt_client;
          mqttUser = mqtt_user;
          mqttPassword = mqtt_password;
          mqttTopicIn = mqtt_topic_in;
          mqttTopicOut = mqtt_topic_out;
          httpServer = http_server;
          httpPort = atoi(http_port);
          postDestination = post_destination;
          Serial.println();
        } else {
          Serial.println(F("Failed to load json config"));
        }
      }
    }
  } else {
    Serial.println(F("Failed to mount FS"));
  }
  ticker.detach();
  configManager();
}

void setup() {
  Serial.begin(MY_BAUD_RATE);
#ifndef MY_DEBUG
  Serial.setDebugOutput(false);
#endif  
  
  for (uint8_t t = 4; t > 0; t--) { // Utile en cas d'OTA ?
      Serial.printf("[SETUP] WAIT %d...\n", t);
      Serial.flush();
      delay(1000);
  }
  
  setPins();
  ticker.attach(0.8, tick);

  if (wifiResetConfig) { // rajouter un bouton
    WiFiManager wifiManager;
    wifiManager.resetSettings();
  }
  
  if (resetConfig) { 
    ticker.attach(0.8, tick);
    Serial.println(F("Resetting config to the inital state"));
    SPIFFS.begin();
    delay(10);
    SPIFFS.format();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    Serial.println(F("System cleared"));
    ticker.detach();
  }
  
  before();
  
  SPIFFS.begin();
  delay(10);
  // check for properties file
  File f = SPIFFS.open(fName, "r");
  if (!f ) {
    // no file exists so lets format and create a properties file
//    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
//    SPIFFS.format();
//    Serial.println("Spiffs formatted");
    f = SPIFFS.open(fName, "w");
    if (!f) {
      Serial.println(F("Resolution file open failed"));
    }
    else {
      // write the defaults to the properties file
      Serial.println(F("====== Writing to resolution file ========="));
      f.println(resolution);
      f.close();
    }
  }
  else {
    // if the properties file exists on startup,  read it and set the defaults
    Serial.println(F("Resolution file exists. Reading."));
    while (f.available()) {
      // read line by line from the file
      String str = f.readStringUntil('\n');
      Serial.println(str);
      resolution = str.toInt();
    }
    f.close();
  }
  
  File f2 = SPIFFS.open(f2Name, "r");
  if (!f2) {
    // no file exists so lets format and create a properties file
//    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
//    SPIFFS.format();
//    Serial.println("Spiffs formatted");
    f2 = SPIFFS.open(f2Name, "w");
    if (!f2) {
      Serial.println(F("FPM file open failed"));
    }
    else {
      // write the defaults to the properties file
      Serial.println(F("====== Writing to FPM file ========="));
      f2.println(fpm);
      f2.close();
    }
  }
  else {
    // if the properties file exists on startup,  read it and set the defaults
    Serial.println(F("FPM file exists. Reading."));

    while (f2.available()) {
      // read line by line from the file;
      String str2 = f2.readStringUntil('\n');
      Serial.println(str2);
      fpm = str2.toInt();
    }
    f2.close();
  }
  
  ////
  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  ticker.detach();
  digitalWrite(BUILTIN_LED, HIGH);
  
  pinMode(CS, OUTPUT);
  SPI.begin();
  SPI.setFrequency(4000000); //4MHz
  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55){
    Serial.println(F("SPI1 interface Error!"));
    while(1);
  }

  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
   if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
    Serial.println("Can't find OV2640 module! pid: " + String(pid));
   // Serial.println("Can't find OV2640 module!");
    else
    Serial.println(F("OV2640 detected."));
 
  //Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  setCamResolution(resolution);
  setFPM(fpm);
  myCAM.InitCAM();
  myCAM.clear_fifo_flag();

  Dir dir = SPIFFS.openDir("/pics");
  while (dir.next()) {
    fileCount++;
  }

  FSInfo fs_info;
  SPIFFS.info(fs_info);

  fileTotalKB = (int)fs_info.totalBytes;
  fileUsedKB = (int)fs_info.usedBytes;
}

void loop() {
if ( ! executeOnce) {
    executeOnce = true; 
    mqttInit();
  }

  if (MqttClient.connected()) {
    ticker.detach();
    MqttClient.loop();
    //yield();
    Capture();
    checkButton();
        ///PREVOIR L'AJOUT D'UN SLEEP POUR USAGE SUR BATTERIE ??
    //if (battery == true) {
    //  sleep...
    //}
  }

 if ( WiFi.status() != WL_CONNECTED) {
    ticker.attach(0.1, tick);
    ++wifiFailCount;
    if (wifiFailCount == 15) {
      ticker.detach();
      configManager();
    }  
  //checkButton();
  }
  
  if (!MqttClient.connected()) {
    ticker.attach(0.3, tick);
    checkButton();
    mqttReconnect();
  }
  
  if ((!MqttClient.connected()) && (WiFi.status() == WL_CONNECTED)) {
    ticker.detach();
    wifiFailCount = 0;
    if (BUILTIN_LED == LOW) {
      digitalWrite(BUILTIN_LED, HIGH);
    }
  }
}
