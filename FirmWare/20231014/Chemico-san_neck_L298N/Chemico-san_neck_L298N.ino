//
//   ChemiCo-SAN neck.
//
//    written by febmarch@eeai.jp. 
//           1 Sep. 2023   
//
//    ChemiCo-SAN neck
//            with L298N motor driver.
//
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include <Servo.h>
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

// PIN assign

const int BATTERY_PIN = A2; // Vout1 --- 10K ohm --- A2 --- 2K ohm --- GND
const float R1 = 10000.0;               //10K ohm
const float R2 = 2000.0;                //2k ohm
const float LOW_BATTERY_LIMIT = 7.0; //2cell LiPo Battery

const int SERVO1_PIN = 3; // PWM
const int SERVO2_PIN = 4; // PWM
const int SERVO3_PIN = 5; // PWM
const int IN1 = A5;   //Neck Motor
const int IN2 = A6;   //Neck Motor
const int IN3 = 0;  // Brest Fun 
const int IN4 = 1;  // Brest Fun 
const int ENA = A4;   //Neck Motor  
const int ENB = 2;   // Brest Fun 

//Servo constant
Servo servo1; // Voice Cords
Servo servo2; // Tongue
Servo servo3; // Lips
static uint8_t loudness = 128;
static uint8_t last_loudness = 128;
static uint8_t voice_cords = 70;
static uint8_t last_voice_cords = 70;
static uint8_t tongue = 30;
static uint8_t last_tongue = 30;
static uint8_t lips = 90;
static uint8_t last_lips = 90;
const uint8_t min_voice_cords = 40;
const uint8_t max_voice_cords = 100;
const uint8_t min_tongue = 5;
const uint8_t max_tongue = 60;
const uint8_t min_lips = 70;
const uint8_t max_lips =110;

//Variable to change the neck motor speed and direction
static uint8_t duty = 0;
static int span = 100;
static int rot_dir = 0;

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

  // Initialize Motor and Servo
  pinMode(BATTERY_PIN, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo3.attach(SERVO3_PIN);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  //Take the battery status
 float battery_voltage = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN, R1, R2);
  Serial.print("Battery voltage: ");
  Serial.print(battery_voltage);
  Serial.println("");

}


void loop() {
  
  //Take the battery status
  float battery_voltage = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN, R1, R2);
  
  //Reset to the default values if the battery level is lower than LOW_BATTERY_LIMIT
  if (battery_voltage < LOW_BATTERY_LIMIT) 
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
    delay(50);
    while (battery_voltage < LOW_BATTERY_LIMIT) 
    {
      battery_voltage = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN, R1, R2);
      delay(100);
    }
  }
  else
  {
    //Send the battery status
    String battery_json = "{\"battery_voltage\":" + String(batteryCheck(IN1, IN2, ENA, BATTERY_PIN, R1, R2)) + "}";
    String http_rsp = httpRequest(battery_json);
    StaticJsonDocument<2000> rsp_json;
    deserializeJson(rsp_json , getHttpResponseBody(http_rsp));
    uint8_t voice_cmd_num = rsp_json["voice_cmd_num"];
    posting_interval = rsp_json["posting_interval"];

    //Moving Head
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
    Serial.print("Rot. Dir: ");
    Serial.print(rot_dir);
    Serial.print(" , Init. Duty: ");
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

    if  (rot_dir == 1)
    {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    } 
    else if (rot_dir == -1)
    {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    }
    else
    {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }
    
    for (int i=duty0; i<duty; i++)
    {
      analogWrite(ENA, i);
      delay(slope0);
    }
    analogWrite(ENA, duty);
    delay(span);
    for (int i=duty; i>duty1; i--)
    {
      analogWrite(ENA, i);
      delay(slope1);
    }
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    delay(10);

    //Voice
    analogWrite(ENB, 0);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    for(int cmd = 0; cmd < voice_cmd_num; cmd++)
    {
      last_loudness = loudness;
      loudness =  rsp_json["loudness"][cmd];
      last_voice_cords = voice_cords;
      voice_cords =  rsp_json["voice_cords"][cmd];
      if (voice_cords < min_voice_cords) voice_cords = min_voice_cords;
      if (voice_cords > max_voice_cords) voice_cords = max_voice_cords;
      last_tongue = tongue;
      tongue = rsp_json["tongue"][cmd];
      if (tongue < min_tongue) tongue = min_tongue;
      if (tongue > max_tongue) tongue = max_tongue;
      last_lips = lips;
      lips = rsp_json["lips"][cmd];
      if (lips < min_lips) lips = min_lips;
      if (lips > max_lips) lips = max_lips;
      uint16_t voice_delay = rsp_json["voice_delay"][cmd];
      Serial.print("Cmd #");
      Serial.print(cmd);
      Serial.print(" , Loudness: ");
      Serial.print(loudness);
      Serial.print(" , Voice cords: ");
      Serial.print(voice_cords);
      Serial.print(" , Tongue: ");
      Serial.print(tongue);
      Serial.print(" , Voice delay: ");
      Serial.print(voice_delay);
      Serial.print(" , Lips: ");
      Serial.println(lips);

      analogWrite(ENB, loudness);
      if (voice_cords != last_voice_cords)
      {
        servo1.write(voice_cords);
      }
      if (tongue != last_tongue)
      {
        servo2.write(tongue);
      }
      if (lips != last_lips)
      {
        servo3.write(lips);
      }
      delay(voice_delay);
    }
    analogWrite(ENB, 0);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);  
    //Take the battery status
    float battery_voltage = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN, R1, R2);
    Serial.print("Battery voltage: ");
    Serial.print(battery_voltage);
    Serial.println("V");
  }    
  //wait
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
    return(rspns);
  }
}

float batteryCheck(int in1, int in2,int en, int bat_pin, float r1, float r2) {
  analogWrite(en, 0);
  digitalWrite(in1, HIGH);  // BREAK
  digitalWrite(in2, HIGH);  // BREAK
  analogWrite(en, 255);
  delay(100);
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
