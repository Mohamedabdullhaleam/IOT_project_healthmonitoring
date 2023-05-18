#include "MAX30105.h"
#include "heartRate.h"
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>

#include <Wire.h>
WiFiClientSecure secured_client;
const char* ssid = "WEDCB108";
const char* password = "Mohamed12369";
#define SERVER  "192.168.1.14"
#define PORT    3000
#define URL     "/"

WebSocketsClient webSocket;
MAX30105 particleSensor;
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
int val ;
float Temperature ;
float beatsPerMinute;
int beatAvg;
long delta;
long irValue;
void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);


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

  irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
     delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 180 / (delta / 1000.0);

   if (beatsPerMinute < 255 && beatsPerMinute >20)
   {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

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


  if (irValue < 50000){
    Serial.print(" No finger?");
    beatsPerMinute = 0 ;
    beatAvg=0; 
    delta =0 ;
    }
  val = analogRead(A0);
  float mv = ( val / 1024.0) * 5000;
  Temperature = mv / 10;
  String Temp = "Temperature : " + String(val) + " °C";
  Serial.println();
  Serial.println(Temp);
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
  jsonDocument["value2"] = beatsPerMinute;
  //jsonDocument["value3"] = beatsPerMinute;
  serializeJson(jsonDocument,jsonData);

  webSocket.sendTXT(jsonData);
}
