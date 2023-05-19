#include "MAX30105.h"
#include "heartRate.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <Wire.h>

WiFiClientSecure secured_client;
WebSocketsClient webSocket;
MAX30105 particleSensor;

const char* ssid     =  "";
const char* password =  "";

#define SERVER     "192.168.1.14"
#define PORT       3000
#define URL        "/"
#define TempPin     A0

const byte RATE_SIZE = 4;      //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];        //Array of heart rates
byte rateSpot = 0;           //Array index
long lastBeat = 0;          //Time at which the last beat occurred
int val;
float Temperature ;
float beatsPerMinute;
int beatAvg;
long delta;
long irValue;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)   //Checks wifi connection
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  webSocket.begin(SERVER, PORT, URL);
  webSocket.onEvent(webSocketEvent);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

void loop() {
  getHeartSensor();
  getTempSensor();
  sendValue();
  webSocket.loop();
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server");
      break;
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");
      break;
  }
}

void sendValue() {
  StaticJsonDocument<256> jsonDocument; 
  String jsonData;
  jsonDocument["value1"] = Temperature ; 
  jsonDocument["value2"] = beatAvg;
  //jsonDocument["value3"] = SPO2;
  serializeJson(jsonDocument, jsonData);    //Convert JSON object to string to send it to server

  webSocket.sendTXT(jsonData);
}

void getHeartSensor() {
  
  irValue = particleSensor.getIR();
  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 180 / (delta / 1000.0);
    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable  (circular array)
      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);


  if (irValue < 50000) {
    Serial.print(" No finger?");
    beatsPerMinute = 0 ;
    beatAvg = 0;
    delta = 0 ;
  }

}

void getTempSensor() {
  //LM35 average reading for human 37
  int val = analogRead(TempPin);
  float mv = ( val / 1024.0) * 5000;
  Temperature = mv / 10;
  String Temp = "Temperature : " + String(Temperature) + " °C";
  Serial.println(Temp);
}
