///////////////////////////////////////////////////////////////////////////////////
//   handle mqtt init, connect, reconnect, callback      //
///////////////////////////////////////////////////////////////////////////////////
void mqttInit() {
  MqttClient.setServer(mqttServer, mqttPort);
  MqttClient.setCallback(mqttCallback);
  //MqttClient.connect((mqttClient),(mqttUser),(mqttPassword));
  //MqttClient.subscribe(mqttTopicIn);
  Serial.println(F("Connected to MQTT server: "));
  Serial.println(mqttServer);
}

boolean connect() {
  //if (!client.connected()) {
    Serial.println(F("Attempting MQTT connection..."));
    if (MqttClient.connect((mqttClient),(mqttUser),(mqttPassword))) {
      Serial.println(F("Connected to MQTT"));
      MqttClient.publish(mqttTopicOut, "Check 1-2 1-2");
      MqttClient.subscribe(mqttTopicIn);
    } 
      return MqttClient.connected();
}

void mqttReconnect() { //blocking loop
  while (!MqttClient.connected()) {
      Serial.print(F("Attempting MQTT connection..."));
      if (MqttClient.connect(mqttClient, mqttUser, mqttPassword)) {
          lastMqttReconnectAttempt=0;
          mqttCount = 0;
          Serial.println(F("Connected to MQTT"));
          delay(100);
          //MqttClient.publish(mqttTopicOut, "Check 1-2 1-2");
          MqttClient.subscribe(mqttTopicIn);
       }
       else {
         Serial.print(F("failed, rc="));
         Serial.print(MqttClient.state());
         Serial.println(F(" try again in 5 seconds"));
     //    ++mqttCount;
       }
//       if (mqttCount == 5) {
//         Serial.println("Connexion MQTT infructueuse après 5 essais --> mode config");
//         lastMqttReconnectAttempt = 0;
//         mqttCount = 0;
//       }
  } 
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    payload[length] = '\0';
    String s = String((char*)payload);
    Serial.print(F("Message arrived ["));
    Serial.print(topic);
    Serial.print(F("] "));
    Serial.println(s);
//    for (int i = 0; i < length; i++) {
//    Serial.print((char)payload[i]);
//    }
    Serial.println();
    if (s == "off") {
        switchOnCam = 0;
    }
    if (s == "on") {
       switchOnCam = 1;
    }
    if (s == "1") {
        t_httpUpdate_return  ret = ESPhttpUpdate.update(otaUrl,"");
        switch (ret) {
          case HTTP_UPDATE_FAILED: 
            Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            Serial.println();
           // return;
            break;
          case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;
          case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
         }
     }
     if (s == "reso0") {
       setCamResolution(0);
       updateResolutionFile();
     }
     if (s == "reso1") {
       setCamResolution(1);
       updateResolutionFile();
     }
     if (s == "reso2") {
       setCamResolution(2);
       updateResolutionFile();
     }
     if (s == "reso3") {
       setCamResolution(3);
       updateResolutionFile();
     }
     if (s == "reso4") {
       setCamResolution(4);
       updateResolutionFile();
     }
     if (s == "reso5") {
       setCamResolution(5);
       updateResolutionFile();
     }
     if (s == "reso6") {
       setCamResolution(6);
       updateResolutionFile();
     }
     if (s == "reso7") {
       setCamResolution(7);
       updateResolutionFile();
     }
     if (s == "reso8") {
       setCamResolution(8);
       updateResolutionFile();
     }
     if (s == "int0") {
       setFPM(0);
       updateFPMFile();
     }
     if (s == "fpm0") {
       setFPM(0);
       updateFPMFile();
     }
     if (s == "fpm1") {
       setFPM(1);
       updateFPMFile();
     }
     if (s == "fpm2") {
       setFPM(2);
       updateFPMFile();
     }
     if (s == "fpm3") {
       setFPM(3);
       updateFPMFile();
     }
     if (s == "fpm4") {
      setFPM(4);
      updateFPMFile();
     }  
}

