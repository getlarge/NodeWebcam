#include <FS.h> 

#include <ArduinoJson.h>  
//#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>        
#include <ESP8266httpUpdate.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h> 
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
ESP8266WiFiMulti WiFiMulti;
WiFiClientSecure MqttWifiClient;
//WiFiServer server(443);
PubSubClient MqttClient(MqttWifiClient);
ArduCAM myCAM(OV2640, CS);
Bounce debouncer = Bounce();
Ticker ticker;

void tick() {
  int state = digitalRead(STATE_LED); 
  digitalWrite(STATE_LED, !state); 
}

void setPins(){
    pinMode(STATE_LED, OUTPUT);
    digitalWrite(STATE_LED, HIGH);
    pinMode(OTA_BUTTON_PIN, INPUT_PULLUP);
    debouncer.attach(OTA_BUTTON_PIN);
    debouncer.interval(3000);
    Serial.println(F("Pins set"));
}

void setReboot() { // Boot to sketch
//    pinMode(STATE_LED, OUTPUT);
//    digitalWrite(STATE_LED, HIGH);
//    pinMode(MY_INCLUSION_MODE_BUTTON_PIN, OUTPUT);
//    digitalWrite(MY_INCLUSION_MODE_BUTTON_PIN, HIGH);
//    pinMode(D8, OUTPUT);
//    digitalWrite(D8, LOW);
    Serial.println(F("Pins set for reboot"));
//    Serial.flush();
//    yield(); yield(); delay(500);
    delay(5000);
    ESP.reset(); //ESP.restart();
    delay(2000);
}

void setDefault() { 
    ticker.attach(2, tick);
    Serial.println(F("Resetting config to the inital state"));
    resetConfig = false;
    SPIFFS.begin();
    delay(10);
    SPIFFS.format();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    delay(100);
    Serial.println(F("System cleared"));
    ticker.detach();
    Serial.println(ESP.eraseConfig());
    setReboot();
}

void checkButton() {
  debouncer.update();
  int value = debouncer.read();
  if (value == LOW) {
      Serial.println(F("Long push detected, ask for config"));
      manualConfig = true;
      configManager();
      value == HIGH;
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

void checkOtaFile() {
  SPIFFS.begin();
  delay(10);
  // check for properties file
  File f = SPIFFS.open(otaFile, "r");
  if (!f ) {
    f = SPIFFS.open(otaFile, "w");
    if (!f) {
      Serial.println(F("OTA file open failed"));
    }
    else {
      Serial.println(F("====== Writing to OTA file ========="));
      f.println(_otaSignal);
      f.close();
    }
  }
  else {
    Serial.println(F("OTA file exists. Reading."));
    while (f.available()) {
      // read line by line from the file
      String str = f.readStringUntil('\n');
      Serial.println(str);
      _otaSignal = str.toInt();
    }
    f.close();
  }
}

void updateOtaFile() {
  File f = SPIFFS.open(otaFile, "w");
  if (!f) {
    Serial.println(F("OTA file open failed"));
  }
  else {
    Serial.println(F("====== Writing to OTA file ========="));

    f.println(_otaSignal);
    Serial.println(F("OTA file updated"));
    f.close();
  }
}

void connectWifi() {
 //   WiFiManager wifiManager;
    String ssid = WiFi.SSID();
    String pass = WiFi.psk();
    WiFi.begin(ssid.c_str(), pass.c_str());
   
    //while (WiFiMulti.run() != WL_CONNECTED) { //use this when using ESP8266WiFiMulti.h
    while (WiFi.status() != WL_CONNECTED) { 
       Serial.print("Attempting Wifi connection....");     
       delay(1000);    
    }
    Serial.println();
    Serial.print("WiFi connected.  IP address:");
    Serial.println(WiFi.localIP());    
}

void getUpdated() {
//  if((WiFi.status() == WL_CONNECTED)) {
//     _MQTT_ethClient.stop();
//     _MQTT_client.disconnect();
    _otaSignal = 0;
    updateOtaFile();
   // Serial.printf("before httpUpdate heap size: %u\n", ESP.getFreeHeap());
    Serial.println(F("Update sketch..."));

    t_httpUpdate_return ret = ESPhttpUpdate.update(otaUrl,"", httpsFingerprint);
    //t_httpUpdate_return ret = ESPhttpUpdate.update("https://app.getlarge.eu/firmware/GatewayMQTT.ino.bin","","1D AE 00 E7 68 70 87 09 A6 1D 27 76 F5 85 C0 F3 AB F2 60 9F");
    //t_httpUpdate_return ret = ESPhttpUpdate.update(httpServer, httpPort, url, currentVersion, httpsFingerprint);
   
    switch(ret) {
      case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          Serial.println();
          break;
      case HTTP_UPDATE_NO_UPDATES:
          Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
          break;
      case HTTP_UPDATE_OK:
          Serial.println(F("HTTP_UPDATE_OK"));
          break;
     }    
//  }
  // ticker.detach();
}
void saveConfigCallback () {
  Serial.println(F("Should save config"));
  shouldSaveConfig = true;
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
 // delay(1000);
  Serial.println(F("Entered config mode"));

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

}

void setup() {
  Serial.begin(MY_BAUD_RATE);
#ifndef MY_DEBUG
  Serial.setDebugOutput(false);
#endif  
 
  checkOtaFile();
  delay(100);
  if (_otaSignal == 1 ) {
     //WiFi.persistent(false);
     String ssid = WiFi.SSID();
     String pass = WiFi.psk();
     WiFiMulti.addAP(ssid.c_str(), pass.c_str());
     while (WiFiMulti.run() != WL_CONNECTED) { //use this when using ESP8266WiFiMulti.h
        Serial.println("Attempting Wifi connection.... ");     
        delay(500);    
     }
    Serial.print("WiFi connected.  IP address:");
    Serial.println(WiFi.localIP());  
 
    getUpdated();
  }
  
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
    setDefault(); 
  }

  getDeviceId();

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
  digitalWrite(STATE_LED, HIGH);
  
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

  Serial.printf("setup heap size: %u\n", ESP.getFreeHeap());
  
  configManager();
  mqttInit();

}

void loop() {
if ( ! executeOnce) {
    executeOnce = true; 
   mqttReconnect();
  }
  
  checkButton();

  if (MqttClient.connected()) {
    wifiFailCount = 0;
    if (STATE_LED == LOW); digitalWrite(STATE_LED, HIGH);
    ticker.detach();
    MqttClient.loop();
    //yield();
    Capture();
        ///PREVOIR L'AJOUT D'UN SLEEP POUR USAGE SUR BATTERIE ??
    //if (battery == true) {
    //  sleep...
    //}
  }

 if ( WiFi.status() != WL_CONNECTED) { // WiFiMulti.run() != WL_CONNECTED
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
    if (mqttFailCount == 15) {
   //   mqttReconnect() = false;
      Serial.println("Connexion MQTT infructueuse après 5 essais --> mode config");
      lastMqttReconnectAttempt = 0;
      mqttFailCount = 0;
      configManager();
     }
  }
}
