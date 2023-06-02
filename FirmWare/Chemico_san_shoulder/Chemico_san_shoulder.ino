//
//   ChemiCo-SAN neck.
//
//    written by febmarch@eeai.jp.
//           23 Mar. 2023
//
//    ChemiCo-SAN shoulder is a program to implement the giraffe long neck
//            for no-arm ChemiCo-SAN's body.
//
#include <WiFiNINA.h>
#include <ArduinoJson.h>
//#include <Servo.h>
#include <Stepper.h>
#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password (use for WPA, or use as key for WEP)
char server[] = SERVER_ADR;  // server address:

int status = WL_IDLE_STATUS;

// Initialize the WiFi client library
WiFiClient client;
uint16_t port = 5000;  // for Flask
String uri = "/shoulder";
unsigned long posting_interval = 200L;  // delay between updates, in milliseconds(200 msec)

// PIN assign

const int R_BATTERY_PIN = A2;           // Vout1 --- 10K ohm --- A2 --- 4.7K ohm --- GND
const int L_BATTERY_PIN = A5;           // Vout1 --- 10K ohm --- A5 --- 4.7K ohm --- GNDc
const float R1 = 10000.0;               //10K ohm
const float R2 = 4700.0;                //4.7k ohm
const float LOW_BATTERY_LIMIT = 7.0;    //2cell LiPo Battery

const int R_IN1 = A3;  // PWM
const int R_IN2 = A4;  // PWM
const int R_IN3 = 0;   // PWM
const int R_IN4 = 1;   // PWM
const int L_IN1 = 2;   // PWM
const int L_IN2 = 3;   // PWM
const int L_IN3 = 4;   // PWM
const int L_IN4 = 5;   // PWM
//const int ENA = 8;
//const int ENB = 9;

const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
// for your motor

// initialize the stepper library on pins;
Stepper rStepper(stepsPerRevolution, R_IN1, R_IN2, R_IN3, R_IN4);
Stepper lStepper(stepsPerRevolution, L_IN1, L_IN2, L_IN3, L_IN4);


//Variable to change the neck motor speed and direction

const int max_pos = 100;
const int min_pos = 0;
const int max_speed = 240;
const int screw_pitch = 5;
uint8_t liftup_speed = 60;
// uint8_t liftdown_speed = 60;
uint16_t current_pos = min_pos;
// unsigned long keep_pos_time = 100L;


void setup() {
  //Serial port initialization
  Serial.begin(9600);
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
  pinMode(R_BATTERY_PIN, INPUT);
  pinMode(R_IN1, OUTPUT);
  pinMode(R_IN2, OUTPUT);
  pinMode(R_IN3, OUTPUT);
  pinMode(R_IN4, OUTPUT);
  pinMode(L_BATTERY_PIN, INPUT);
  pinMode(L_IN1, OUTPUT);
  pinMode(L_IN2, OUTPUT);
  pinMode(L_IN3, OUTPUT);
  pinMode(L_IN4, OUTPUT);
  // pinMode(ENA, OUTPUT);
  // pinMode(ENB, OUTPUT);

  // servo1.attach(SERVO1_PIN);
  // servo2.attach(SERVO2_PIN);
  // servo3.attach(SERVO3_PIN);

  digitalWrite(R_IN1, LOW);
  digitalWrite(R_IN2, LOW);
  digitalWrite(R_IN3, LOW);
  digitalWrite(R_IN4, LOW);
  digitalWrite(L_IN1, LOW);
  digitalWrite(L_IN2, LOW);
  digitalWrite(L_IN3, LOW);
  digitalWrite(L_IN4, LOW);
  // analogWrite(ENA, 0);
  // analogWrite(ENB, 0);

  rStepper.setSpeed(liftup_speed);
  lStepper.setSpeed(liftup_speed);

  //Take the battery status
  float r_battery_voltage = (float)batteryCheck(R_IN1, R_IN2, R_BATTERY_PIN, R1, R2);
  float l_battery_voltage = (float)batteryCheck(L_IN1, L_IN2, L_BATTERY_PIN, R1, R2);
  Serial.print("Right Battery voltage: ");
  Serial.print(r_battery_voltage);
  Serial.print("V , Left Battery voltage: ");
  Serial.print(l_battery_voltage);
  Serial.println("V");
}


void loop() {

  //Take the battery status
  float r_battery_voltage = (float)batteryCheck(R_IN1, R_IN2, R_BATTERY_PIN, R1, R2);
  float l_battery_voltage = (float)batteryCheck(L_IN1, L_IN2, L_BATTERY_PIN, R1, R2);

  //Reset to the default values if the battery level is lower than LOW_BATTERY_LIMIT
  if ((r_battery_voltage < LOW_BATTERY_LIMIT) || (l_battery_voltage < LOW_BATTERY_LIMIT)) {
    Serial.println(" ");
    Serial.println("WARNING: LOW BATTERY");
    Serial.println("ALL SYSTEMS DOWN");
    Serial.print("Right Battery voltage: ");
    Serial.print(r_battery_voltage);
    Serial.print("V , Left Battery voltage: ");
    Serial.print(l_battery_voltage);
    Serial.println("V");
    digitalWrite(R_IN1, LOW);
    digitalWrite(R_IN2, LOW);
    digitalWrite(R_IN3, LOW);
    digitalWrite(R_IN4, LOW);
    digitalWrite(L_IN1, LOW);
    digitalWrite(L_IN2, LOW);
    digitalWrite(L_IN3, LOW);
    digitalWrite(L_IN4, LOW);
    //    analogWrite(ENA, 0);
    //    analogWrite(ENB, 0);
    while ((r_battery_voltage < LOW_BATTERY_LIMIT) || (l_battery_voltage < LOW_BATTERY_LIMIT)) {
      r_battery_voltage = (float)batteryCheck(R_IN1, R_IN2, R_BATTERY_PIN, R1, R2);
      l_battery_voltage = (float)batteryCheck(L_IN1, L_IN2, L_BATTERY_PIN, R1, R2);
    }
  } else {
    //Send the battery status
    String battery_json = "{\"battery_voltage0\":" + String(batteryCheck(R_IN1, R_IN2, R_BATTERY_PIN, R1, R2)) + 
                          ",\"battery_voltage1\":" + String(batteryCheck(L_IN1, L_IN2, L_BATTERY_PIN, R1, R2)) + 
                          "}";
    String http_rsp = httpRequest(battery_json);
    StaticJsonDocument<1000> rsp_json;
    deserializeJson(rsp_json, getHttpResponseBody(http_rsp));
    posting_interval = rsp_json["posting_interval"];

    //Lift Up
    current_pos = rsp_json["duty"];
    if (current_pos > max_pos) current_pos = max_pos;
    liftup_speed = rsp_json["slope"];
    if (liftup_speed > max_speed) liftup_speed = max_speed;
    int direction = rsp_json["direction"];
    // keep_pos_time = rsp_json["span"];
    // liftdown_speed = rsp_json["slope1"];
    // if (liftdown_speed > max_speed) liftdown_speed = max_speed;
    Serial.print("Lift Up Pos.: ");
    Serial.print(current_pos);
    Serial.print("mm , Lift Up Speed: ");
    Serial.print(liftup_speed);
    Serial.print("rpm , Direction: ");
    Serial.println(direction);
    // Serial.print("rpm , Keep Time: ");
    // Serial.print(keep_pos_time);
    // Serial.print("msec , Lift Down Speed: ");
    // Serial.print(liftdown_speed);
    // Serial.println("rpm");

    int steps = stepsPerRevolution * current_pos / screw_pitch;
    if (direction > 0) {
      rStepper.setSpeed(liftup_speed);
      lStepper.setSpeed(liftup_speed);
      for (int i = 0; i < steps; i++) {
        rStepper.step(1);
        lStepper.step(1);
      }
    }
    else if (direction < 0) {
      rStepper.setSpeed(liftup_speed);
      lStepper.setSpeed(liftup_speed);
      for (int j = 0; j < steps; j++) {
        rStepper.step(-1);
        lStepper.step(-1);
      }
    }
    digitalWrite(R_IN1, LOW);
    digitalWrite(R_IN2, LOW);
    digitalWrite(R_IN3, LOW);
    digitalWrite(R_IN4, LOW);
    digitalWrite(L_IN1, LOW);
    digitalWrite(L_IN2, LOW);
    digitalWrite(L_IN3, LOW);
    digitalWrite(L_IN4, LOW);
    delay(100);

    //Take the battery status
    float r_battery_voltage = (float)batteryCheck(R_IN1, R_IN2, R_BATTERY_PIN, R1, R2);
    float l_battery_voltage = (float)batteryCheck(L_IN1, L_IN2, L_BATTERY_PIN, R1, R2);
    Serial.print("Right Battery voltage: ");
    Serial.print(r_battery_voltage);
    Serial.print("V , Left Battery voltage: ");
    Serial.print(l_battery_voltage);
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
  block_end = false;
  while (pos < http_rsp_end && !block_end) {
    String separator = http_rsp.substring(pos, pos + 4);
    if (separator == "\r\n\r\n") {
      pos += 3;
      block_end = true;
    }
    pos++;
  }
  // get Body
  body = http_rsp.substring(pos, http_rsp_end);
  return (body);
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
    return (rspns);
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    return (rspns);
  }
}

float batteryCheck(int in1, int in2, int bat_pin, float r1, float r2) {
  //  analogWrite(ENA, 0);
  digitalWrite(in1, HIGH);  // BREAK
  digitalWrite(in2, HIGH);  // BREAK
                            //  analogWrite(ENA, 255);
  delay(10);
  float battery_voltage = (float)analogRead(bat_pin) / 1024.0 * 3.3 * (r1 + r2) / r2;
  digitalWrite(in1, LOW);  // OFF
  digitalWrite(in2, LOW);  // OFF
                           //  analogWrite(ENA, 0);
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
