//
//   ChemiCo-SAN hand.
//
//    written by febmarch@eeai.jp. 
//           8 Sep. 2023   
//
//    ChemiCo-SAN hand (mild vacuum chuck)
//            with L298N motor driver.
//
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include <Servo.h>
#include <SPI.h>                         // Needed for legacy versions of Arduino.
#include <BME280Spi.h>
#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char server[] = SERVER_ADR;// server address:

int status = WL_IDLE_STATUS;

// Initialize the WiFi client library
WiFiClient client;
uint16_t port = 5000; // for Flask
String uri = "/handR";
const unsigned long INITIAL_posting_interval = 2000L;            // delay between updates, in milliseconds(2000 msec)
const unsigned long MINIMUM_posting_interval = 100L;             // minimum delay
unsigned long posting_interval = INITIAL_posting_interval;

// PIN assign

const int BATTERY_PIN = A2; // Vout1 --- 10K ohm --- A2 --- 2K ohm --- GND
const float R1 = 10000.0;               //10K ohm
const float R2 = 2000.0;                //2k ohm
const float LOW_BATTERY_LIMIT = 7.0; //2cell LiPo Battery

const int CS = 7; // for SPI Chip Select
const int SERVO1_PIN = 3; // PWM
const int SERVO2_PIN = 4; // PWM
const int ENA = A3;  // Fan1 PWM 
const int IN1 = A0;  // Fan1
const int IN2 = A1;  // Fan1
const int IN3 = 11;   // Fan2
const int IN4 = 12;   // Fan2  
const int ENB = 6;   // Fan2 PMW 
const int ENC = A4;  // Fan3 (PWM ENA2)
const int IN5 = A5;  // Fan3 (IN1-2)
const int IN6 = A6;  // Fan3 (IN2-2)
const int IN7 = 0;   // Fan4 (IN3-2)
const int IN8 = 1;   // Fan4 (IN4-2)
const int END = 2;   // Fan4 (PWM ENB2)

//Servo constant
Servo servo1; // Wrist1
Servo servo2; // Wrist2
static uint8_t last_wrist1 =90;
static uint8_t last_wrist2 =90;
const uint8_t min_wrist1 = 10;
const uint8_t max_wrist1 =170;
const uint8_t min_wrist2 = 10;
const uint8_t max_wrist2 =170;

//BME280 settings
BME280Spi::Settings settings(CS); // Default : forced mode, standby time = 1000 ms
                                          //           Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
BME280Spi bme(settings);

//Fan Limitter
const uint8_t max_duty = 255;
static uint8_t duty1,duty2,duty3,duty4;
static uint8_t rot_dir1,rot_dir2,rot_dir3,rot_duty4;
static bool cont1=false,cont2=false,cont3=false,cont4=false;
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

  //Initialize BME280
  SPI.begin();
  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  switch(bme.chipModel())
  {
    case BME280::ChipModel_BME280:
      Serial.println("Found BME280 sensor! Success.");
      break;
    case BME280::ChipModel_BMP280:
      Serial.println("Found BMP280 sensor! No Humidity available.");
      break;
    default:
      Serial.println("Found UNKNOWN sensor! Error!");
  }

  // Initialize Motor and Servo
  pinMode(BATTERY_PIN, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IN5, OUTPUT);
  pinMode(IN6, OUTPUT);
  pinMode(IN7, OUTPUT);
  pinMode(IN8, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(ENC, OUTPUT);
  pinMode(END, OUTPUT);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  digitalWrite(IN5, LOW);
  digitalWrite(IN6, LOW);
  digitalWrite(IN7, LOW);
  digitalWrite(IN8, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  analogWrite(ENC, 0);
  analogWrite(END, 0);

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
    digitalWrite(IN5, LOW);
    digitalWrite(IN6, LOW);
    digitalWrite(IN7, LOW);
    digitalWrite(IN8, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    analogWrite(ENC, 0);
    analogWrite(END, 0);
    delay(50);
    while (battery_voltage < LOW_BATTERY_LIMIT) 
    {
      battery_voltage = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN, R1, R2);
      delay(100);
    }
  }
  else
  {
    //Send the battery status and environment data
    float temp(NAN), hum(NAN), pres(NAN);
    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);
    bme.read(pres, temp, hum, tempUnit, presUnit);
    String env_json = "{\"battery_voltage\":" + String(batteryCheck(IN1, IN2, ENA, BATTERY_PIN, R1, R2)) +
                      ",\"temperature\":" + String(temp) + ",\"temp_unit\":" + String(tempUnit) +
                      ",\"pressure\":" + String(pres) + ",\"pres_unit\":" + String(presUnit) +
                      ",\"humidity\":" + String(hum) +
                      "}";
    String http_rsp = httpRequest(env_json);
    StaticJsonDocument<2000> rsp_json;
    deserializeJson(rsp_json , getHttpResponseBody(http_rsp));
    uint8_t voice_cmd_num = rsp_json["voice_cmd_num"];
    posting_interval = rsp_json["posting_interval"];

    //Moving Wrist
    int wrist1 = rsp_json["pos1"];
    if (wrist1 > max_wrist1) wrist1 = max_wrist1;
    if (wrist1 < min_wrist1) wrist1 = min_wrist1;
    int wrist2 = rsp_json["pos2"];
    if (wrist2 > max_wrist2) wrist2 = max_wrist2;
    if (wrist2 < min_wrist2) wrist2 = min_wrist2;

    Serial.print("Last Wrist1 Pos.: ");
    Serial.print(last_wrist1);
    Serial.print(" , Wrist1 Pos.: ");
    Serial.print(wrist1);
    Serial.print(" , Last Wrist2 Pos.: ");
    Serial.print(last_wrist2);
    Serial.print(" , Wrist2 Pos.: ");
    Serial.println(wrist2);

    if  (wrist1 != last_wrist1)
    {
      servo1.write(wrist1);
      last_wrist1 = wrist1;
    }
    if  (wrist2 != last_wrist2)
    {
      servo2.write(wrist2);
      last_wrist2 = wrist2;
    } 
   
    //Rotating Fan 1,2,3,4
    span = rsp_json["span"]; 
    cont1 = rsp_json["cont1"];
    int duty_in = rsp_json["duty1"];
    if (duty1 > max_duty) duty1 = max_duty; else duty1 = duty_in;
    int rot_dir1 = rsp_json["rot_dir1"];
    cont2 = rsp_json["cont2"];
    duty_in = rsp_json["duty2"];
    if (duty2 > max_duty) duty2 = max_duty; else duty2 = duty_in;
    int rot_dir2 = rsp_json["rot_dir2"];
    cont3 = rsp_json["cont3"];
    duty_in = rsp_json["duty3"];
    if (duty3 > max_duty) duty3 = max_duty; else duty3 = duty_in;
    int rot_dir3 = rsp_json["rot_dir3"];
    cont4 = rsp_json["cont4"];
    duty_in = rsp_json["duty4"];
    if (duty4 > max_duty) duty4 = max_duty; else duty4 = duty_in;
    int rot_dir4 = rsp_json["rot_dir4"];
    Serial.print("Keep Time: ");
    Serial.print(span);
    Serial.print(" ,Duty1: ");
    Serial.print(duty1);
    Serial.print(" , Rot Dir1: ");
    Serial.print(rot_dir1);
    Serial.print(" , Continue: ");
    Serial.print(cont1);
    Serial.print(" ,Duty2: ");
    Serial.print(duty2);
    Serial.print(" , Rot Dir2: ");
    Serial.print(rot_dir2);
    Serial.print(" , Continue: ");
    Serial.print(cont3);
    Serial.print(" ,Duty3: ");
    Serial.print(duty3);
    Serial.print(" , Rot Dir3: ");
    Serial.print(rot_dir3);
    Serial.print(" , Continue: ");
    Serial.print(cont3);
    Serial.print(" ,Duty4: ");
    Serial.print(duty4);
    Serial.print(" , Rot Dir4: ");
    Serial.print(rot_dir4);
    Serial.print(" , Continue: ");
    Serial.print(cont4);
    Serial.println("");

    if  (rot_dir1 == 1)
    {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    } 
    else if (rot_dir1 == -1)
    {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    }
    else if (rot_dir1 == 0)
    {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }
    if  (rot_dir2 == 1)
    {
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } 
    else if (rot_dir2 == -1)
    {
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    }
    else if (rot_dir2 == 0)
    {
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
    }
    if  (rot_dir3 == 1)
    {
      digitalWrite(IN5, HIGH);
      digitalWrite(IN6, LOW);
    } 
    else if (rot_dir3 == -1)
    {
      digitalWrite(IN5, LOW);
      digitalWrite(IN6, HIGH);
    }
    else if (rot_dir3 == 0)
    {
      digitalWrite(IN5, LOW);
      digitalWrite(IN6, LOW);
    }
    if  (rot_dir4 == 1)
    {
      digitalWrite(IN7, HIGH);
      digitalWrite(IN8, LOW);
    } 
    else if (rot_dir4 == -1)
    {
      digitalWrite(IN7, LOW);
      digitalWrite(IN8, HIGH);
    }
    else if (rot_dir4 == 0)
    {
      digitalWrite(IN7, LOW);
      digitalWrite(IN8, LOW);
    }


    analogWrite(ENA, duty1);
    analogWrite(ENB, duty2);
    analogWrite(ENC, duty3);
    analogWrite(END, duty4);

    delay(span);

    if (!cont1) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      analogWrite(ENA, 0);
    }
    if (!cont2) {
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      analogWrite(ENB, 0);
    }
    if (!cont3) {
      digitalWrite(IN5, LOW);
      digitalWrite(IN6, LOW);
      analogWrite(ENC, 0);
    }
    if (!cont4) {
      digitalWrite(IN7, LOW);
      digitalWrite(IN8, LOW);
      analogWrite(END, 0);
    }
  
    //Take the battery status
    float battery_voltage = (float)batteryCheck(IN1, IN2, ENA, BATTERY_PIN, R1, R2);
    Serial.print("Battery voltage: ");
    Serial.print(battery_voltage);
    Serial.println("V");
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
