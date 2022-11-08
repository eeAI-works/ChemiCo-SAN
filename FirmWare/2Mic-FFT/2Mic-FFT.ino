//
//    Wifi dual Microphone FFT sender
//
//    written by febmarch@eeai.jp for ChemiCo-SAN ear. (Experimental)
//           7 Nov. 2022   
//
//    Since this program uses the AdafruitZeroFFT library, the BSD-3 license applies similarly.
//      https://github.com/adafruit/Adafruit_ZeroFFT/blob/master/license.txt
//

#include"Adafruit_ZeroFFT.h"
#include <WiFiNINA.h>
#include <ArduinoJson.h>

#define FFT_MAX 1024
/*set this to a power of 2. 
 * Lower = faster, lower resolution
 * Higher = slower, higher resolution
  */
#define DATA_SIZE 2048

#define RIGHT_EAR_PIN 20; //A5
#define LEFT_EAR_PIN 21; //A6
uint8_t ear_pin = RIGHT_EAR_PIN; //default Ear is right
String ear_name ="right";
int16_t ear[DATA_SIZE];
unsigned long start_sampling;
unsigned long end_sampling;

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;

// Initialize the WiFi client library
WiFiClient client;

// server address:
char server[] = "febmarch04";
uint16_t port = 5000; // for Flask
String uri = "/listener";

unsigned long posting_interval = 200L;            // delay between updates, in milliseconds(200 msec)

// the setup routine runs once when you press reset:
void setup() {
  AdcBooster();
  analogReadResolution(12); // 12bit resolution
  Serial.begin(115200);
  
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(5000);
  }
  // you're connected now, so print out the status:
  printWifiStatus();
}

void loop() {
  delay(posting_interval);
  
  // ear sampling
  unsigned long start_FFT = millis();
  int32_t avg = 0;
  start_sampling = millis();
  for(int i=0; i<DATA_SIZE; i++){
    int16_t pick = analogRead(ear_pin);
    avg += pick;
    ear[i] = pick;
  }
  end_sampling = millis();
  //remove DC offset and gain up to 16 bits
  avg = avg/DATA_SIZE;
  for(int i=0; i<DATA_SIZE; i++) {
    ear[i] = (ear[i] - avg) * 16;
  }
  //run the FFT
  ZeroFFT(ear, DATA_SIZE);
  unsigned long end_FFT = millis();

  unsigned long start_http_request = millis();
  String http_rsp = httpRequest(jsonEncode(ear_name));
  unsigned long end_http_request = millis();
  StaticJsonDocument<200> rsp_json;
  deserializeJson(rsp_json , getHttpResponseBody(http_rsp));
  posting_interval = rsp_json["posting_interval"];
  const char* ear_nam2 = rsp_json["which_ear"];

  Serial.print(ear_name);
  Serial.print(" t=");
  Serial.print(end_sampling - start_sampling);
  Serial.print("msec , freq.=");
  Serial.print((float)DATA_SIZE  / ((float)end_sampling - (float)start_sampling));
  Serial.print("kHz , FFT time=");
  Serial.print( end_FFT - start_FFT);
  Serial.print("msec , HTTPreq=");
  Serial.print( end_http_request - start_http_request);
  Serial.print("msec , Next Posting : ");
  Serial.print(posting_interval);
  Serial.println("msec later");

  ear_name = String(ear_nam2);
  if(ear_name == "right") {
    ear_pin = RIGHT_EAR_PIN;
  } else {
    ear_pin = LEFT_EAR_PIN;
    ear_name = "left";
  }
}

String jsonEncode(String ear_slct) {
  String json_out = "";
  json_out.concat("{\"which\":\"");
  json_out.concat(ear_slct);
  json_out.concat("\",\"sample_time\":");
  json_out.concat(end_sampling - start_sampling);
  json_out.concat(",\"sample_size\":");
  json_out.concat(DATA_SIZE);
  json_out.concat(",\"ear\":[");
  for(int i=0; i<DATA_SIZE/2; i++) {
    if(i != 0)  json_out.concat(",");
      json_out.concat(ear[i]);
    }
  json_out.concat("]}");
  return(json_out);
}

String getHttpResponseBody(String http_rsp) {

  String body = "";
  int pos = 0;
  int http_rsp_end = http_rsp.length();
  bool block_end = false;

  // skip header
  block_end =false;
  while(pos < http_rsp_end && !block_end) {
    String separator = http_rsp.substring(pos,pos+4);
    if (separator == "\r\n\r\n") {
      pos += 3;
      block_end =true;
    }
    pos++;
  } 
  // get Body
  body = http_rsp.substring(pos , http_rsp_end);
  return(body);
}

// this method makes a HTTP connection to the server:
String httpRequest(String postdata) {
  // close any connection before send a new request.
  // This will free the socket on the NINA module
  client.stop();
  
  String rspns = "";

  // if there's a successful connection:
  if (client.connect(server, port)) {
    Serial.println("connecting...");
    // send the HTTP POST request:
    client.print("POST http://");
    client.print(server);
    client.print(":");
    client.print(port);
    client.print(uri);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.print(server);
    client.print(":");
    client.println(port);
    client.println("Content-type: application/json");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Accept-Encoding: gzip, deflate");
    client.print("Content-Length: ");
    client.println(postdata.length());
    client.println("Connection: close");
    client.println();
    client.println(postdata);

    // an HTTP request ends with a blank line
    // boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        rspns.concat(String(c));
      }
    }
    return(rspns);
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    return(rspns);
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void AdcBooster()
{
  ADC->CTRLA.bit.ENABLE = 0;                     // Disable ADC
  while( ADC->STATUS.bit.SYNCBUSY == 1 );        // Wait for synchronization
  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV64 |  // Divide Clock by 32.
                   ADC_CTRLB_RESSEL_10BIT;       // Result on 10 bits
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 |   // 1 sample
                     ADC_AVGCTRL_ADJRES(0x00ul); // Adjusting result by 0
  ADC->SAMPCTRL.reg = 0x00;                      // Sampling Time Length = 0
  ADC->CTRLA.bit.ENABLE = 1;                     // Enable ADC
  while( ADC->STATUS.bit.SYNCBUSY == 1 );        // Wait for synchronization
} // AdcBooster
