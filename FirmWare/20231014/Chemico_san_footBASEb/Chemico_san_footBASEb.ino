//
//   ChemiCo-SAN footBASEb
//
//    written by febmarch@eeai.jp. 
//           20 Jul. 2023   
//
//    ChemiCo-SAN footBASE a and b are moving station for ChemiCo-SAN
//            with L298N motor driver.
//
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include "HX711.h"
#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char server[] = SERVER_ADR;// server address:

int status = WL_IDLE_STATUS;

// Initialize the WiFi client library
WiFiClient client;
uint16_t port = 5000; // for Flask
String uri = "/foot24";
const unsigned long INITIAL_posting_interval = 2000L;            // delay between updates, in milliseconds(2000 msec)
const unsigned long MINIMUM_posting_interval = 100L;             // minimum delay
unsigned long posting_interval = INITIAL_posting_interval;
// PIN assign

const int BATTERY_PIN24 = A4;           // Vout1 --- 10K ohm --- A4 --- 2K ohm --- GND
const float R1 = 10000.0;               //10K ohm
const float R2 = 2000.0;                //2k ohm
const float LOW_BATTERY_LIMIT = 7.0;    //2cell LiPo Battery

const int IN1 = 1;  // Tire 1
const int IN2 = 2;  // Tire 1
const int IN3 = 3;   // Tire 3 
const int IN4 = 4;   // Tire 3 
const int ENA = 0;  // PWM  
const int ENB = 5;   // PWM 
//HX711 
HX711 scale2;
HX711 scale4;
const int dataPin2 = 7;
const int dataPin4 = 8;
uint8_t clockPin = 6;

//Limitter and weights
const uint8_t max_duty = 255;
static float w2 = 0.0, w4 = 0.0; 
static uint8_t duty2,duty4;
static uint8_t rot_dir2,rot_dir4;
static bool cont2=false,cont4=false;
static int span;

void setup() 
{
  //Serial port initialization
  Serial.begin(115200);
  //while (!Serial);
  delay(100);
  
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

  // Initialize Motor24
  pinMode(BATTERY_PIN24, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  //Initialize Scale24
  Serial.print("LIBRARY VERSION: ");
  Serial.println(HX711_LIB_VERSION);
  Serial.println();

  scale2.begin(dataPin2, clockPin);
  scale4.begin(dataPin4, clockPin);

  Serial.print("UNITS(2): ");
  Serial.println(scale2.get_units(10));
  scale2.set_scale(420.0983);       // load cell factor 5 KG
  scale2.tare();
  Serial.print("UNITS(4): ");
  Serial.println(scale4.get_units(10));
  scale4.set_scale(420.0983);       // load cell factor 5 KG
  scale4.tare();

  //Take the battery status
  float battery_voltage24 = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN24, R1, R2);
  Serial.print("Battery voltage: ");
  Serial.print(battery_voltage24);
  Serial.println("V");

}


void loop() {
  
  //Take the battery status
  float battery_voltage24 = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN24, R1, R2);
  
  //Reset to the default values if the battery level is lower than LOW_BATTERY_LIMIT
  if (battery_voltage24 < LOW_BATTERY_LIMIT)
  {
    Serial.println(" ");
    Serial.println("WARNING: LOW BATTERY");
    Serial.println("ALL SYSTEMS DOWN");
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    delay(100);
    while (battery_voltage24 < LOW_BATTERY_LIMIT)
    {
      battery_voltage24 = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN24, R1, R2);
      delay(100);
    }
  }
  else
  {
    w2 = scale2.get_units(3);
    w4 = scale4.get_units(3);
    battery_voltage24 = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN24, R1, R2);
    Serial.print("Battery voltage: ");
    Serial.print(battery_voltage24);
    Serial.println("V");
    //Send the battery status and weights
    String pickup_json = "{\"battery_voltage24\":" + String(battery_voltage24) + 
                         ",\"scale2\":" +  String(w2) +
                         ",\"scale4\":" +  String(w4) +
                         "}";
    String http_rsp = httpRequest(pickup_json);
//    Serial.println(http_rsp);
    StaticJsonDocument<1000> rsp_json;
    deserializeJson(rsp_json , getHttpResponseBody(http_rsp));
    unsigned long posting_interval_in = rsp_json["posting_interval"];
    if (posting_interval_in > MINIMUM_posting_interval) posting_interval = posting_interval_in;

    //Moving Three Tires 2 4
    span = rsp_json["span"];
    
    cont2 = rsp_json["cont2"];
    int duty_in = rsp_json["duty2"];
    if (duty2 > max_duty) duty2 = max_duty; else duty2 = duty_in;
    int rot_dir2 = rsp_json["rot_dir2"];
    cont4 = rsp_json["cont4"];
    duty_in = rsp_json["duty4"];
    if (duty4 > max_duty) duty4 = max_duty; else duty4 = duty_in;
    int rot_dir4 = rsp_json["rot_dir4"];
    Serial.print("Keep Time: ");
    Serial.print(span);
    Serial.print(" ,Duty2: ");
    Serial.print(duty2);
    Serial.print(" , Rot Dir2: ");
    Serial.print(rot_dir2);
    Serial.print(" , Continue: ");
    Serial.print(cont2);
    Serial.print(" ,Duty4: ");
    Serial.print(duty4);
    Serial.print(" , Rot Dir4: ");
    Serial.print(rot_dir4);
    Serial.print(" , Continue: ");
    Serial.print(cont4);
    Serial.println("");

    if  (rot_dir2 == 1)
    {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    } 
    else if (rot_dir2 == -1)
    {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    }
    else if (rot_dir2 == 0)
    {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }
    if  (rot_dir4 == 1)
    {
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } 
    else if (rot_dir4 == -1)
    {
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    }
    else if (rot_dir4 == 0)
    {
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
    }

    analogWrite(ENA, duty2);
    analogWrite(ENB, duty4);

    delay(span);

    if (!cont2) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      analogWrite(ENA, 0);
    }
    if (!cont4) {
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      analogWrite(ENB, 0);
    }

  }  
  //wait
  Serial.print("Posting Interval = ");
  Serial.print(posting_interval);
  Serial.println("msec");
  delay(posting_interval);
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
    status = WL_CONNECT_FAILED;
    // attempt to connect to WiFi network:
    for (int i=0; i<10; i++) {
      if(status == WL_CONNECTED) break;
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(ssid, pass);
      // wait 5 seconds for connection:
      delay(5000);
    }
    posting_interval = INITIAL_posting_interval;
    return(rspns);
  }
}

float batteryCheck(int in1, int in2,int en, int bat_pin, float r1, float r2) {
  analogWrite(en, 0);
  digitalWrite(in1, HIGH);  // BREAK
  digitalWrite(in2, HIGH);  // BREAK
  analogWrite(en, 255);
  delay(50);
  float battery_voltage = (float)analogRead(bat_pin) / 1024.0 * 3.3 * (r1 + r2) / r2;
  digitalWrite(in1, LOW);  // OFF
  digitalWrite(in2, LOW);  // OFF
  analogWrite(en, 0);
  // battery_voltage = 7.8; //dummy
  return (battery_voltage);
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
