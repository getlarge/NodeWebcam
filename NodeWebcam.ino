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

#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <Bounce2.h>

#include "memorysaver.h" // config file in Arducam library
#include "config.h"
#if !(defined ESP8266 )
#error Please select the ArduCAM ESP8266 UNO board in the Tools/Board
#endif

//WiFiClientSecure httpClient;
WiFiClientSecure MqttWifiClient;
PubSubClient MqttClient(MqttWifiClient);
ArduCAM myCAM(OV2640, CS);
Bounce debouncer = Bounce();

void setPins(){
//    pinMode(BUILTIN_LED, OUTPUT);
    pinMode(OTA_BUTTON_PIN, INPUT_PULLUP);
    debouncer.attach(OTA_BUTTON_PIN);
    debouncer.interval(2000);
    Serial.println("pins set");
}

void checkButton() {
  debouncer.update();
  int value1 = debouncer.read();
  if (value1 == LOW) {
    Serial.println("Long push detected, ask for config");
    before();
  }
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void before() {
  Serial.println();
  Serial.println("mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
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
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  
  WiFiManager wifiManager;
  String script;
  script += "<script>";
  script += "document.addEventListener('DOMContentLoaded', function() {";
  script +=     "var params = window.location.search.substring(1).split('&');";
  script +=     "for (var param of params) {";
  script +=         "param = param.split('=');";
  script +=         "try {";
  script +=             "document.getElementById( param[0] ).value = param[1];";
  script +=         "} catch (e) {";
  script +=             "console.log('WARNING param', param[0], 'not found in page');";
  script +=         "}";
  script +=     "}";
  script += "});";
  script += "</script>";
  wifiManager.setCustomHeadElement(script.c_str());
  wifiManager.setDebugOutput(true);
  wifiManager.setBreakAfterConfig(true);
  //wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_client);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_http_server);
  wifiManager.addParameter(&custom_http_port);
  wifiManager.setMinimumSignalQuality();  
  wifiManager.setConfigPortalTimeout(600);
  char msgBuffer[10];         
  char *espChipId;
  float chipId = ESP.getChipId();
  espChipId = dtostrf(chipId, 10, 0, msgBuffer);
  strcpy(deviceId,devicePrefix); 
  strcat(deviceId,espChipId);
  
  if ((MqttClient.connected()) || ((!MqttClient.connected()) && (WiFi.status() != WL_CONNECTED))) {
    wifiManager.startConfigPortal(deviceId, devicePass);
  }
  
  if (!wifiManager.autoConnect(deviceId, devicePass)) {
    Serial.println("Connection failure --> Timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  
  Serial.println("Connected");
  if (shouldSaveConfig) {
    strcpy(mqtt_server, custom_mqtt_server.getValue()); 
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_client, custom_mqtt_client.getValue()); 
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    strcpy(http_server, custom_http_server.getValue()); 
    strcpy(http_port, custom_http_port.getValue());
    Serial.println("Saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_client"] = mqtt_client;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;
    json["http_server"] = http_server;
    json["http_port"] = http_port;
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
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("Failed to open config file");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }
  
  Serial.print("IP locale : ");
  Serial.println(WiFi.localIP());
  Serial.println("Config User : ");
  Serial.print(mqttClient);
  Serial.print(" | ");
  Serial.println(mqttUser);
  Serial.println("Config MQTT : ");
  Serial.print(mqttServer);
  Serial.print(":");
  Serial.println(mqttPort);
  Serial.print(mqttTopicIn);
  Serial.print(" | ");
  Serial.println(mqttTopicOut);
  Serial.println("Config HTTP: ");
  Serial.print(httpServer);
  Serial.print(":");
  Serial.print(httpPort);
  Serial.println(postDestination);
  Serial.println("--------------------");
  Serial.println();
  delay(200);
}

void setup() {
  Serial.begin(BAUD_RATE);
  setPins();
  
  for (uint8_t t = 4; t > 0; t--) { // Utile en cas d'OTA ?
      Serial.printf("[SETUP] WAIT %d...\n", t);
      Serial.flush();
      delay(1000);
  }
  
  if (wifiResetConfig) { // rajouter un bouton
    WiFiManager wifiManager;
    wifiManager.resetSettings();
  }
  
  if (resetConfig) {
    SPIFFS.begin();
    delay(10);
    SPIFFS.format();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
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
      Serial.println("resolution file open failed");
    }
    else {
      // write the defaults to the properties file
      Serial.println("====== Writing to resolution file =========");
      f.println(resolution);
      f.close();
    }
  }
  else {
    // if the properties file exists on startup,  read it and set the defaults
    Serial.println("Resolution file exists. Reading.");
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
      Serial.println("fpm file open failed");
    }
    else {
      // write the defaults to the properties file
      Serial.println("====== Writing to FPM file =========");
      f2.println(fpm);
      f2.close();
    }
  }
  else {
    // if the properties file exists on startup,  read it and set the defaults
    Serial.println("FPM file exists. Reading.");

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
 
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  pinMode(CS, OUTPUT);
  SPI.begin();
  SPI.setFrequency(4000000); //4MHz

  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55){
    Serial.println("SPI1 interface Error!");
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
    Serial.println("OV2640 detected.");
 
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

  //  if (resetConfig) { // rajouter un debounce avec long délai
//    SPIFFS.begin();
//    delay(10);
//    SPIFFS.format();
//    WiFiManager wifiManager;
//    wifiManager.resetSettings();
//  }

  if (MqttClient.connected()) {
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
  //ticker.attach(0.1, tick);
  checkButton();
  }
  
  if (!MqttClient.connected()) {
  //ticker.attach(0.3, tick);
    checkButton();
    mqttReconnect();
  }
}
