/*

  modified by febmarch@eeai.jp for ChemiCo-SAN eye.
           23 Oct. 2022
  Copyright in this code is governed by the following licenses:

*/

// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//    FRAMESIZE_96x96,    // 96x96
//    FRAMESIZE_QQVGA,    // 160x120 ZOOM 1 PAN 0 TILT 0
//    FRAMESIZE_QQVGA2,   // 128x160
//    FRAMESIZE_QCIF,     // 176x144
//    FRAMESIZE_HQVGA,    // 240x176 ZOOM 1.5 PAN 40 TILT 28
//    FRAMESIZE_240x240,  // 240x240
//    FRAMESIZE_QVGA,     // 320x240 ZOOM 2 PAN 80 TILT 60
//    FRAMESIZE_CIF,      // 400x296 ZOOM 2.5 PAN 120 TILT 88
//    FRAMESIZE_VGA,      // 640x480 ZOOM 4 PAN 240 TILT 180
//    FRAMESIZE_SVGA,     // 800x600 ZOOM 5 PAN 320 TILT 240
//    FRAMESIZE_XGA,      // 1024x768 ZOOM 6.4 PAN 432 TILT 324
//    FRAMESIZE_SXGA,     // 1280x1024
//    FRAMESIZE_UXGA,     // 1600x1200
//    FRAMESIZE_QXGA,     // 2048*1536
//    FRAMESIZE_INVALID

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "esp_camera.h"

WebServer server(80);

const int max_x = 800;
const int max_y = 600;
const int px = 48;
const int py = 48;
const int max_zoom = 12; // min(max_x / px , max_y / py) 
const String pix_format = "RGB888";

void handleRoot() {
  server.send(200, "text/plain", "Hello ESP-EYE vision server.");
}

void handleParam() {
  int zoom,pix_size,pan,tilt;

  zoom = atoi(server.arg("zoom").c_str());
  if (!server.hasArg("zoom")) zoom = 1;
  if (zoom < 1) zoom = 1;
  if (zoom > max_zoom) zoom = max_zoom;
  pix_size = max_zoom - zoom + 1;
  
  pan = atoi(server.arg("pan").c_str());
  if (!server.hasArg("pan")) pan = max_x /2 - pix_size * px / 2;;
  if (pan < 0) pan = 0;
  if (pan > max_x - pix_size * px) pan = max_x - pix_size * px;

  tilt = atoi(server.arg("tilt").c_str());
  if (!server.hasArg("tilt")) tilt = max_y / 2 - pix_size * py / 2;
  if (tilt < 0) tilt = 0;
  if (tilt > max_y - pix_size * py) tilt = max_y - pix_size * py;
  
  String jsonOutput = "";
  jsonOutput.concat("{");
  jsonOutput.concat("\"x\":");
  jsonOutput.concat(max_x);
  jsonOutput.concat(",\"y\":");
  jsonOutput.concat(max_y);
  jsonOutput.concat(",\"px\":");
  jsonOutput.concat(px);
  jsonOutput.concat(",\"py\":");
  jsonOutput.concat(py);
  jsonOutput.concat(",\"max_zoom\":");
  jsonOutput.concat(max_zoom);
  jsonOutput.concat(",\"pan\":");
  jsonOutput.concat(pan);
  jsonOutput.concat(",\"tilt\":");
  jsonOutput.concat(tilt);
  jsonOutput.concat(",\"pix_size\":");
  jsonOutput.concat(pix_size);
  jsonOutput.concat(",\"pix_format\":\"");
  jsonOutput.concat(pix_format);
  jsonOutput.concat("\"}");
  
  server.send(200, "application/json", jsonOutput);
}


void handleVision() {
  int zoom,pix_size,pan,tilt;
  
  zoom = atoi(server.arg("zoom").c_str());
  if (!server.hasArg("zoom")) zoom = 1;
  if (zoom < 1) zoom = 1;
  if (zoom > max_zoom) zoom = max_zoom;
  pix_size = max_zoom - zoom + 1;
  
  pan = atoi(server.arg("pan").c_str());
  if (!server.hasArg("pan")) pan = max_x / 2 - pix_size * px / 2;
  if (pan < 0) pan = 0;
  if (pan > max_x - pix_size * px) pan = max_x - pix_size * px;

  tilt = atoi(server.arg("tilt").c_str());
  if (!server.hasArg("tilt")) tilt = max_y / 2 - pix_size * py / 2;
  if (tilt < 0) tilt = 0;
  if (tilt > max_y - pix_size * py) tilt = max_y - pix_size * py;
  
  String jsonOutput = "";
  jsonOutput.concat("{");
  jsonOutput.concat("\"x\":");
  jsonOutput.concat(px);
  jsonOutput.concat(",\"y\":");
  jsonOutput.concat(py);
  jsonOutput.concat(",\"pan\":");
  jsonOutput.concat(pan);
  jsonOutput.concat(",\"tilt\":");
  jsonOutput.concat(tilt);
  jsonOutput.concat(",\"pix_size\":");
  jsonOutput.concat(pix_size);
  jsonOutput.concat(",\"pixel\": [");
  
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
 
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");    
    server.send(500, "text/plain", "Camera capture failed\n");
    return;
  }

  digitalWrite(22,HIGH);
 
  uint8_t * pixel_buf = NULL;
  pixel_buf = fb->buf;
  int x,y,x_sub,y_sub;
  int r0,g0,b0,pos;

// Averager  
  for(y=0; y<py; y++){
    for(x=0; x<px; x++){
      r0=0;
      g0=0;
      b0=0;
      for(y_sub=0; y_sub<pix_size; y_sub++){
        for(x_sub=0; x_sub<pix_size; x_sub++){
          pos=((tilt + y_sub + y * pix_size) * max_x + pan + x_sub + x * pix_size)*3;
          b0 += pixel_buf[pos];
          g0 += pixel_buf[pos+1];
          r0 += pixel_buf[pos+2];
        }
      } 
      r0 /= pix_size * pix_size;
      r0 = min(r0 , 255);
      g0 /= pix_size * pix_size;
      g0 = min(g0 , 255);
      b0 /= pix_size * pix_size;
      b0 = min(b0 , 255);
      if (y==0 && x == 0) {
        jsonOutput.concat("[");
      } else {
        jsonOutput.concat(",[");
      }
      jsonOutput.concat(r0);
      jsonOutput.concat(",");
      jsonOutput.concat(g0);
      jsonOutput.concat(",");
      jsonOutput.concat(b0);
      jsonOutput.concat("]");
    }
  }

  esp_camera_fb_return(fb);

   digitalWrite(22,LOW);
  
  jsonOutput.concat("]}");
  server.send(200, "application/json", jsonOutput);
}


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void startJSONserver(void) {

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/param", HTTP_GET, handleParam);

  server.on("/vision", HTTP_GET, handleVision);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loopJSONserver(void) {
  server.handleClient();
}
