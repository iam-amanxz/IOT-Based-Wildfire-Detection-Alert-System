/*******************************************************
*    Wildfire Detection and Alert System
*    Copyright (C) 2020 Husnul Aman.
*******************************************************/

// -------------Library Imports------------- //
#include "ThingSpeak.h"
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Adafruit_ADS1015.h>
#include <Wire.h>

// ------------------Used Items------------------ //
// ******DHT11 Humidity Sensor******    //  Gets Temperature and Humidity
// ******Ultrasonic Sensor******        //  Measures distance to animals/birds from device
// ******LDR Sensor******               //  Measures light intensity 
// ******Soil Moisture Sensor******     //  Measures soil moisture
// ******LED******                      //  Indicates alerts
// ******Buzzer******                   //  Alerts when danger

// -------------Sensor PIN Setups------------- //
//D0 = GPIO16 : RED_PIN
//D1 = GPIO5  : ADSS1115(SCL)
//D2 = GPIO4  : ADSS1115(SDA)
//D3 = GPIO0  : DHT11
//D4 = GPIO2  : ULTRASONIC(TRIGGER)
//D5 = GPIO14 : ULTRASONIC(ECHO)
//D6 = GPIO12 : BUZZER
//D7 = GPIO13 : GREEN_PIN
//D8 = GPIO15 : BLUE_PIN

// Humidity
#define DHTPIN 0
DHT dht(DHTPIN, DHT11);

// Ultrasonic
const int TRIGGER = 2;
const int ECHO = 14;

const int RED_PIN = D0;
const int GREEN_PIN = D7;
const int BLUE_PIN = D8;
const int BUZZER = 12;



// -------------Wifi Configurations------------- //
const char* ssid = "YOUR_SSID";                              
const char* password = "YOUR_PASSWORD";   
WiFiClient client;

// -------------Thingspeak Configurations------------- //
String apiKey = "THINGSPEAK_API_KEY";                                     
const char* server = "api.thingspeak.com";

Adafruit_ADS1115 ads;

void setup(){
  Serial.begin(115200);
  delay(10);

  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT);
  
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  ads.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125
  ads.begin();
  dht.begin();
  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.print("..........");
  Serial.println();

  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  }
  Serial.println("WiFi connected");
  Serial.println();
}

void loop(){
  long val = 0; // (LDR, Soil)
  int light_level = 0, soilMoisture_level = 0; // (LDR, Soil)
  
  //--------------------------DHT11(Humidity)------------------------- //  
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); 

  //--------------------------Ultrasonic------------------------- //
  long T;
  float distanceCM; 
  
  digitalWrite(TRIGGER, LOW);
  delay(1);
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);
  T = pulseIn(ECHO, HIGH);
  distanceCM = T * 0.034;
  distanceCM = distanceCM / 2;

  //--------------------------LDR------------------------- //  
  for (int number = 0; number < 10; number++)   
  {  
    light_level = ads.readADC_SingleEnded(0);  
    val = val + light_level;  
    delay(100);  
  }  
  light_level = val/10;
  float voltage0 = (light_level * 0.125)/1000;
  val = 0; 

  //--------------------------Soil Moisture------------------------- //
  for (int number = 0; number < 10; number++)   
  {  
    soilMoisture_level = ads.readADC_SingleEnded(1);  
    val = val + soilMoisture_level;  
    delay(100);  
  }  
  soilMoisture_level = val/10;
  float voltage1 = (soilMoisture_level * 0.125)/1000;
  
  //-------------------------- Alerts------------------------- //
  digitalWrite(BUZZER, LOW);
  setColor(0, 255, 0);
  
  if(humidity < 30 || temperature > 45 || soilMoisture_level > 50000){
    setColor(255, 0, 0);
    digitalWrite(BUZZER, HIGH);
    delay(1000);
    digitalWrite(BUZZER, LOW);
    delay(1000); // continous blinking beeps when fire danger
  }
  if(distanceCM < 5 ){
    setColor(0, 0, 255);
    digitalWrite(BUZZER, HIGH); // continous linear beeps when somthing aproaches the device
  }
  
  //--------------------------Serial Readings------------------------- //

//  Humidity Sensor------------------------------//
  Serial.print("Humidity(C): ");
  Serial.print(humidity);
  Serial.print("\t Temperature(C): ");
  Serial.println(temperature);

//  Ultrasinic Sensor------------------------------//
  Serial.print("Distance(CM): ");
  Serial.println(distanceCM); 

//  LDR Sensor------------------------------//
  Serial.print("LDR Average Value: ");  
  Serial.print(light_level);  
  Serial.print("\t LDR Voltage Output: ");   
  Serial.println(voltage0, 6); 
  if(light_level < 5000){
  Serial.println("It seems to be dark, DARK MODE IS ON");  
  }
  
//  Soil Moisture Sensor------------------------------//
  Serial.print("Soil Moisture Average Value: ");   
  Serial.print(soilMoisture_level);  
  Serial.print("\t Soil Moisture Voltage Output: ");   
  Serial.println(voltage1, 6); 
  
  Serial.println("\n\n");

  //--------------------------thingspeak-------------------------
  if (client.connect(server,80)) { // "184.106.153.149" or api.thingspeak.com

    String sendData = apiKey+"&field1="+String(humidity)+"&field2="+String(temperature)+"&field3="+String(distanceCM)+"&field4="+String(light_level)+"&field5="+String(soilMoisture_level)+"\r\n\r\n"; 
    
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(sendData.length());
    client.print("\n\n\n\n\n\n\n\n");
    client.print(sendData);
    }
    client.stop();
    delay(20000);
}

// -------------LED Configurations------------- //
void setColor(int redValue, int greenValue, int blueValue){
    analogWrite(RED_PIN, redValue);
    analogWrite(GREEN_PIN, greenValue);
    analogWrite(BLUE_PIN, blueValue);
}
