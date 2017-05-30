///////////////////////////////////////////////////////////////////////////////////
//    function to modifiy, save, settings for connection to wifi and server      //
///////////////////////////////////////////////////////////////////////////////////
void configManager() {
  Serial.println(F("Wifi config mode opening"));
  ticker.attach(0.5, tick);
  wifiCount++;
//  debouncer2.attach(MY_INCLUSION_MODE_BUTTON_PIN);
//  debouncer2.interval(5000);
  
  WiFiManager wifiManager;
#ifndef MY_DEBUG 
  wifiManager.setDebugOutput(false);
#endif
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setBreakAfterConfig(true);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setMinimumSignalQuality(10);
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
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_client);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_http_server);
  wifiManager.addParameter(&custom_http_port);

  getDeviceId();
  wifiConfigMode = 1;
  
if ((wifiCount > 1 && MqttClient.connected() && WiFi.status() == WL_CONNECTED) || (wifiCount > 1 && !MqttClient.connected() && WiFi.status() == WL_CONNECTED)) {
    Serial.println(F("Manual config access"));
    wifiManager.setConfigPortalTimeout(wifiConfigTime);
    wifiManager.startConfigPortal(deviceId, devicePass); 
  }
  
  else if (!wifiManager.autoConnect(deviceId, devicePass)) {
    Serial.println(F("Connection failure --> Timeout"));
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  Serial.print(F("Config mode counter : "));
  Serial.println(wifiCount);
  Serial.println(F("Connected"));
  if (shouldSaveConfig) {
    strcpy(mqtt_server, custom_mqtt_server.getValue()); 
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_client, custom_mqtt_client.getValue()); 
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    strcpy(http_server, custom_http_server.getValue()); 
    strcpy(http_port, custom_http_port.getValue());
    Serial.println(F("Saving config"));
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
      Serial.println(F("Failed to open config file"));
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }

  Serial.println(F("Wifi config mode closed"));
  wifiConfigMode = 0;
  ticker.detach();
  digitalWrite(BUILTIN_LED, HIGH);
  
  Serial.println(F("Config User : "));
  Serial.print(mqttClient);
  Serial.print(" | ");
  Serial.println(mqttUser);
  Serial.println(F("Config MQTT : "));
  Serial.print(mqttServer);
  Serial.print(":");
  Serial.println(mqttPort);
  Serial.print(mqttTopicIn);
  Serial.print(" | ");
  Serial.println(mqttTopicOut);
  Serial.println(F("Config HTTP: "));
  Serial.print(httpServer);
  Serial.print(":");
  Serial.print(httpPort);
  Serial.println(postDestination);
  Serial.println(F("--------------------"));
  Serial.println();
  }
