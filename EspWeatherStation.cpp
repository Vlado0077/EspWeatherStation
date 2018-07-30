/*
 * EspWeatherStation.cpp
 *
 *  Created on: 2. 7. 2018
 *      Author: Vladimir
 */

#if 1
#include "../private.h"
#else
#include "public.h"
#endif

#define ESP8266_NODEMCU


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include "MyNtp.h"

#define DEBUG 100

ESP8266WiFiMulti wifiMulti;

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

File fsUploadFile;

WiFiUDP udp;
MyNtp mytime;

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);
  return false;
}

void handleNotFound(){
  if(!handleFileRead(server.uri())){
    server.send(404, "text/plain", "404: File Not Found");
  }
}

void handleFileUpload(){
  HTTPUpload& upload = server.upload();
  String path;
  if(upload.status == UPLOAD_FILE_START){
    path = upload.filename;
    if(!path.startsWith("/")) path = "/"+path;
    if(!path.endsWith(".gz")) {
      String pathWithGz = path+".gz";
      if(SPIFFS.exists(pathWithGz))
         SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");
    path = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {
      fsUploadFile.close();
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      if (payload[0] == '#') {
        uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
      } else if (payload[0] == 'R') {
      } else if (payload[0] == 'N') {
      }
      break;
  }
}




void startWiFi() {

    wifiMulti.addAP(mySSID1, myPASS1);
    wifiMulti.addAP(mySSID2, myPASS2);
    wifiMulti.addAP(mySSID3, myPASS3);

    Serial.println("Connecting");
    while (wifiMulti.run() != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
	    delay(250);
	    Serial.print('.');
	}
	Serial.println("\r\n");
	Serial.print("Connected to ");
	Serial.println(WiFi.SSID());
	Serial.print("IP address:\t");
	Serial.print(WiFi.localIP());
	Serial.println("\r\n");

}


void startOTA() {
  ArduinoOTA.setHostname(otaNane);
  ArduinoOTA.setPassword(otaPass);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}

void startSPIFFS() {
  SPIFFS.begin();
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

void startWebSocket() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started.");
}

void startMDNS() {
  MDNS.begin(mdmsName);
  Serial.print("mDNS responder started: http://");
  Serial.print(mdmsName);
  Serial.println(".local");
}

void startServer() {
  server.on("/edit.html",  HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started.");
}



void setup() {
	Serial.begin(115200);
	delay(200);

	startWiFi();
	startOTA();
	startSPIFFS();
	startWebSocket();
	startMDNS();

}

void loop() {
	mytime.update(udp);
	webSocket.loop();
	server.handleClient();
	ArduinoOTA.handle();

	Serial.println(mytime.getFormattedTime());

	delay(1000);
}
