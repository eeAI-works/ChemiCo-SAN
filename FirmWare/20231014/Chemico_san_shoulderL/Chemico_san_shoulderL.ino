//
//   ChemiCo-SAN Shoulder
//
//    written by febmarch@eeai.jp. 
//           8 Aug. 2023   
//
//    ChemiCo-SAN Shoulder R and L are moving arms for ChemiCo-SAN
//            with L298N motor driver.
//
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include <Stepper.h>
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
String uri = "/shoulderL";
const unsigned long INITIAL_posting_interval = 2000L;            // delay between updates, in milliseconds(2000 msec)
const unsigned long MINIMUM_posting_interval = 100L;             // minimum delay
unsigned long posting_interval = INITIAL_posting_interval;
// PIN assign

const uint8_t BATTERY_PIN = A2;           // Vout1 --- 10K ohm --- A4 --- 2K ohm --- GND
const float R1 = 10000.0;               //10K ohm
const float R2 = 2000.0;                //2k ohm
const float LOW_BATTERY_LIMIT = 7.0;    //2cell LiPo Battery

const uint8_t IN1 = 0;  // PWM
const uint8_t IN2 = 1;  // PWM
const uint8_t IN3 = 2;   // PWM 
const uint8_t IN4 = 3;   // PWM
const uint8_t SERVO1_PIN = 4; // PWM
const uint8_t SERVO2_PIN = 5; // PWM
const uint8_t SERVO3_PIN = A3; // PWM
const uint8_t SERVO4_PIN = A4; // PWM

const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
// for your motor
// initialize the stepper library on pins 8 through 11:
Stepper shoulderStepper(stepsPerRevolution, IN1, IN2, IN3, IN4);

//Variable to change the shoukder stepper speed and direction

const int max_pos = 200;
const int min_pos = 0;
const int max_speed = 240;
const int screw_pitch = 5;
uint8_t liftup_speed = 120;
// uint8_t liftdown_speed = 60;
uint16_t current_pos = min_pos;
// unsigned long keep_pos_time = 100L;

//Servo constant
Servo servo1; // Shoulder
Servo servo2; // Elbow
Servo servo3; //Wing1
Servo servo4; //Wing2
static uint8_t shoulder = 90;
static uint8_t last_shoulder = 90;
static uint8_t elbow = 90;
static uint8_t last_elbow = 90;
static uint8_t wing1 = 90;
static uint8_t last_wing1 = 90;
static uint8_t wing2 = 90;
static uint8_t last_wing2 = 90;
const uint8_t max_shoulder =170;
const uint8_t min_shoulder= 10;
const uint8_t max_elbow = 170;
const uint8_t min_elbow = 10;
const uint8_t max_wing1 =110;
const uint8_t min_wing1= 70;
const uint8_t max_wing2 = 110;
const uint8_t min_wing2 = 70;

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

  // set the speed at 120 rpm:
  shoulderStepper.setSpeed(liftup_speed);

  //Initialize Servo
  servo1.attach(SERVO1_PIN); 
  servo2.attach(SERVO2_PIN); 
  servo3.attach(SERVO3_PIN); 
  servo4.attach(SERVO4_PIN); 

  //Take the battery status
  float battery_voltage = (float)batteryCheck(IN1, IN2, BATTERY_PIN, R1, R2);
  Serial.print("Battery voltage: ");
  Serial.print(battery_voltage);
  Serial.println("V");

}


void loop() {
  
  //Take the battery status
  float battery_voltage = (float)batteryCheck(IN1, IN2, BATTERY_PIN, R1, R2);
  
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
    delay(100);
    while (battery_voltage < LOW_BATTERY_LIMIT)
    {
      battery_voltage = (float)batteryCheck(IN1, IN2, BATTERY_PIN, R1, R2);
      delay(100);
    }
  }
  else
  {
    battery_voltage = (float)batteryCheck(IN1, IN2, BATTERY_PIN, R1, R2);
    Serial.print("Battery voltage: ");
    Serial.print(battery_voltage);
    Serial.println("V");
    //Send the battery status and weights
    String pickup_json = "{\"battery_voltage\":" + String(battery_voltage) + 
                         "}";
    String http_rsp = httpRequest(pickup_json);
    // Serial.println(http_rsp);
    StaticJsonDocument<1000> rsp_json;
    deserializeJson(rsp_json , getHttpResponseBody(http_rsp));
    unsigned long posting_interval_in = rsp_json["posting_interval"];
    if (posting_interval_in > MINIMUM_posting_interval) posting_interval = posting_interval_in;

  //Lift Up
    int next_pos = rsp_json["duty"];
    if (next_pos > max_pos) next_pos = max_pos;
    if (next_pos < min_pos) next_pos = min_pos;
    liftup_speed = rsp_json["slope"];
    if (liftup_speed > max_speed) liftup_speed = max_speed;
    int direction = rsp_json["direction"];
    Serial.print("Current Pos.: ");
    Serial.print(current_pos);
    Serial.print("mm Next Pos.: ");
    Serial.print(next_pos);
    Serial.print("mm , Lift Up Speed: ");
    Serial.print(liftup_speed);
    Serial.print("rpm , Direction: ");
    Serial.println(direction);

    int steps = stepsPerRevolution * (next_pos - current_pos) / screw_pitch;
    shoulderStepper.setSpeed(liftup_speed);
    shoulderStepper.step(- direction * steps);

    current_pos = next_pos;
  
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

// Moving Arm
    shoulder =  rsp_json["pos1"];
    if (shoulder < min_shoulder) shoulder = min_shoulder;
    if (shoulder > max_shoulder) shoulder = max_shoulder;
    elbow =  rsp_json["pos2"];
    if (elbow < min_elbow) elbow = min_elbow;
    if (elbow > max_elbow) elbow = max_elbow;
    wing1 =  rsp_json["pos3"];
    if (wing1 < min_wing1) wing1 = min_wing1;
    if (wing1> max_wing1) wing1= max_wing1;
    wing2 =  rsp_json["pos4"];
    if (wing2 < min_wing2) wing2 = min_wing2;
    if (wing2 > max_wing2) wing2 = max_wing2;
    Serial.print("Shoulder Pos.: ");
    Serial.print(shoulder);
    Serial.print(" , Elbow Pos.: ");
    Serial.print(elbow);
    Serial.print(" , Wing1 Pos.: ");
    Serial.print(wing1);
    Serial.print(" , Wing2 Pos.: ");
    Serial.println(wing2);

    if (last_shoulder != shoulder )
      {
        servo1.write(shoulder);
        last_shoulder = shoulder;
      }
    
    if (last_elbow != elbow )
      {
        servo2.write(elbow);
        last_elbow = elbow;
      }
    
    if (last_wing1 != wing1 )
      {
        servo3.write(wing1);
        last_wing1 = wing1;
      }
    
    if (last_wing2 != wing2 )
      {
        servo4.write(wing2);
        last_wing2 = wing2;
      }
    
    delay(100);
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

float batteryCheck(int in1, int in2,int bat_pin, float r1, float r2) {
  digitalWrite(in1, HIGH);  // BREAK
  digitalWrite(in2, HIGH);  // BREAK
  delay(10);
  float battery_voltage = (float)analogRead(bat_pin) / 1024.0 * 3.3 * (r1 + r2) / r2;
  digitalWrite(in1, LOW);  // OFF
  digitalWrite(in2, LOW);  // OFF
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
