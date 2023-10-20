#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char server[] = SERVER_ADR;// server address:

int status = WL_IDLE_STATUS;

// Initialize the WiFi client library
WiFiClient client;
uint16_t port = 5000; // for Flask
String uri = "/neck";
unsigned long posting_interval = 200L;            // delay between updates, in milliseconds(200 msec)

#include <ArduinoMotorCarrier.h>
#define INTERRUPT_PIN 6

//Variable to store the battery voltage
static int batteryVoltage;

//Variable to change the motor speed and direction
static int duty = 0;
static int span = 100;
static int rot_dir = 0;

void setup() 
{
  //Serial port initialization
  Serial.begin(115200);
  //while (!Serial);
  delay(100);
  //Establishing the communication with the Motor Carrier
  if (controller.begin()) 
    {
      Serial.print("MKR Motor Connected connected, firmware version ");
      Serial.println(controller.getFWVersion());
    } 
  else 
    {
      Serial.println("Couldn't connect! Is the red LED blinking? You may need to update the firmware with FWUpdater sketch");
      while (1);
    }

  // Reboot the motor controller; brings every value back to default
  Serial.println("reboot");
  controller.reboot();
  delay(500);
  
  //Take the battery status
  float batteryVoltage = (float)battery.getConverted();
  Serial.print("Battery voltage: ");
  Serial.print(batteryVoltage);
  Serial.print("V, Raw ");
  Serial.println(battery.getRaw());

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 5 seconds for connection:
    delay(5000);
  }
  // you're connected now, so print out the status:
  printWifiStatus();

}


void loop() {
  
  //Take the battery status
  float batteryVoltage = (float)battery.getConverted();
  
  //Reset to the default values if the battery level is lower than 7 V
  if (batteryVoltage < 7) 
  {
    Serial.println(" ");
    Serial.println("WARNING: LOW BATTERY");
    Serial.println("ALL SYSTEMS DOWN");
    M1.setDuty(0);
    M2.setDuty(0);
    M3.setDuty(0);
    M4.setDuty(0);
    while (batteryVoltage < 7) 
    {
      batteryVoltage = (float)battery.getConverted();
    }
  }
  else
  {
    String bat_json = "{\"battery_raw\":" + String(battery.getRaw()) + "}";
    String http_rsp = httpRequest(bat_json);
    StaticJsonDocument<200> rsp_json;
    deserializeJson(rsp_json , getHttpResponseBody(http_rsp));
    posting_interval = rsp_json["posting_interval"];
    duty = rsp_json["duty"];
    span = rsp_json["span"];
    int rot_dir_in = rsp_json["rot_dir"];
    if (rot_dir_in > 0) rot_dir = 1;
    if (rot_dir_in == 0) rot_dir = 0;
    if (rot_dir_in < 0) rot_dir = -1;
    int duty0 = rsp_json["duty0"];
    int slope0 = rsp_json["slope0"];
    int duty1 = rsp_json["duty1"];
    int slope1 = rsp_json["slope1"];
    Serial.print("Init. Duty: ");
    Serial.print(duty0);
    Serial.print(" , Inc. slope: ");
    Serial.print(slope0);
    Serial.print(" , Top Duty: ");
    Serial.print(duty);
    Serial.print(" , Keep Time: ");
    Serial.print(span);
    Serial.print(" , Dec. slope: ");
    Serial.print(slope1);
    Serial.print(" , Rump Duty: ");
    Serial.println(duty1);
     
    for (int i=duty0; i<duty; i++)
    {
      M1.setDuty(i*rot_dir);
      M2.setDuty(i*rot_dir);
      M3.setDuty(i*rot_dir);
      delay(slope0);
    }
    M1.setDuty(duty*rot_dir);
    M2.setDuty(duty*rot_dir);
    M3.setDuty(duty*rot_dir);
    delay(span);
    for (int i=duty; i>duty1; i--)
    {
      M1.setDuty(i*rot_dir);
      M2.setDuty(i*rot_dir);
      M3.setDuty(i*rot_dir);
      delay(slope1);
    }
    M1.setDuty(0);
    M2.setDuty(0);
    M3.setDuty(0);
    delay(10);

  //Take the battery status
  float batteryVoltage = (float)battery.getConverted();
  Serial.print("Battery voltage: ");
  Serial.print(batteryVoltage);
  Serial.print("V, Raw ");
  Serial.println(battery.getRaw());
    
  //Keep active the communication between MKR board & MKR Motor Carrier
  //Ping the SAMD11
  controller.ping();
  //wait
  delay(50);
  }
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
