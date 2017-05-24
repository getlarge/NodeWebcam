void Capture() {
  if (((millis() - lastPic) > minDelayBetweenframes) && (switchOnCam)) {
    lastPic = millis();
    digitalWrite(BUILTIN_LED, LOW);
    myCAM.clear_fifo_flag();
    myCAM.start_capture();
    Serial.println(" Start capture");

    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
    size_t len = myCAM.read_fifo_length();
    if (len >= 0x07ffff) {
        Serial.println("Over size.");
        return;
    }else if (len == 0 ) {
        Serial.println("Size is 0.");
        return;
    }
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
    #if !(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM))
    SPI.transfer(0xFF);
    #endif
    digitalWrite(BUILTIN_LED, HIGH);
    Serial.println("Ready to send capture");
    sendPic(len);
    myCAM.CS_HIGH();
    
    unsigned long timeToSend = millis() - lastPic;
    Serial.print(" in ");
    Serial.print(timeToSend);
    Serial.println(" ms");

  }
}
    
void sendPic(int len) {
  //WiFiClientSecure httpClient;
  if (MqttWifiClient.connect(httpServer, httpPort)) {
  //if (httpClient.connect(url, httpPort)) {
      Serial.print("Send ");
      Serial.print(millis());
      String formStart;
      String formEnd;

      formStart += "--boundary\r\n";
      formStart += "Content-Disposition: form-data; name=\"login\"\r\n";
      formStart += "\r\n";
      formStart += mqttUser;
      formStart += "\r\n";
      formStart += "--boundary\r\n";
      formStart += "Content-Disposition: form-data; name=\"password\"\r\n";
      formStart += "\r\n";
      formStart += mqttPassword;
      formStart += "\r\n";
      formStart += "--boundary\r\n";
      formStart += "Content-Disposition: form-data; name=\"image\"; filename=\"Capture.jpg\"\r\n";
      formStart += "Content-Type: image/jpeg\r\n";
      formStart += "\r\n";
      formEnd += "\r\n";
      formEnd += "--boundary--\r\n";
      int length123 = formStart.length() + len + formEnd.length();
//      httpClient.print("POST ");
//      httpClient.print(postDestination);
//      httpClient.print(" HTTP/1.1\r\n");
//      httpClient.print("Host: " + String(httpServer) + "\r\n");
//      httpClient.print("Content-Type: multipart/form-data; boundary=boundary\r\n");
//      httpClient.print("Content-Length: " + String(length123) + "\r\n");
//      httpClient.print("\r\n");
//      httpClient.print(formStart);

      MqttWifiClient.print("POST ");
      MqttWifiClient.print(postDestination);
      MqttWifiClient.print(" HTTP/1.1\r\n");
      MqttWifiClient.print("Host: " + String(httpServer) + "\r\n");
      MqttWifiClient.print("Content-Type: multipart/form-data; boundary=boundary\r\n");
      MqttWifiClient.print("Content-Length: " + String(length123) + "\r\n");
      MqttWifiClient.print("\r\n");
      MqttWifiClient.print(formStart);
      while (len) {
          size_t will_copy = (len < bufferSize) ? len : bufferSize;
          SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
         // if (!httpClient.connected()) break;
          if (!MqttWifiClient.connected()) break;
//          httpClient.write(&buffer[0], will_copy);
            MqttWifiClient.write(&buffer[0], will_copy);

          len -= will_copy;
      }
      MqttWifiClient.print(formEnd);

//    while (!httpClient.available())
//
//    while (httpClient.available()) {
//
//      char str=httpClient.read();
//      Serial.print(str);
//    }
      MqttWifiClient.stop();
      Serial.println(" OK");
  }
}

