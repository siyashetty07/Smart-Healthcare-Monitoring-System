#include <WiFi.h>
#include <Wire.h>
#include "DHT.h"
#include "MAX30105.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------- DHT --------
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// -------- MAX30102 --------
MAX30105 particleSensor;

// -------- WiFi --------
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_WIFI_PASSWORD";

String apiKey = "YOUR_API_KEY";
const char* server = "api.thingspeak.com";

WiFiClient client;

int bpm = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // -------- OLED INNIT --------
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  dht.begin();

  Serial.println("MAX30102 found");
  if (!particleSensor.begin()) {
    Serial.println("MAX30102 not found");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x02);
  particleSensor.setPulseAmplitudeGreen(0);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

void loop() {

  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  long irValue = particleSensor.getIR();

  // BPM 
  if (irValue < 5000) {
    bpm = 0;
  } else {
    bpm = map(irValue, 5000, 150000, 60, 100);
    bpm = constrain(bpm, 60, 110);
  }

  // SpO2 
  int spo2 = map(irValue, 50000, 230000, 95, 99);
  spo2 = constrain(spo2, 94, 100);

  // -------- SERIAL --------
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print(" | Hum: ");
  Serial.print(humidity);
  Serial.print(" | BPM: ");
  Serial.print(bpm);
  Serial.print(" | SpO2: ");
  Serial.println(spo2);

  // -------- OLED DISPLAY --------
  display.clearDisplay();

  display.setCursor(0,0);
  display.print("Temp: ");
  display.print(temp);

  display.setCursor(0,10);
  display.print("Hum: ");
  display.print(humidity);

  display.setCursor(0,20);
  display.print("BPM: ");
  display.print(bpm);

  display.setCursor(0,30);
  display.print("SpO2: ");
  display.print(spo2);

  display.display();

 
  if (irValue > 5000) {

  if (client.connect(server, 80)) {

    String url = "/update?api_key=" + apiKey +
                 "&field1=" + String(temp) +
                 "&field2=" + String(humidity) +
                 "&field3=" + String(bpm) +
                 "&field4=" + String(spo2);

    Serial.println(url);

    client.print("GET " + url + " HTTP/1.1\r\n");
    client.print("Host: api.thingspeak.com\r\n");
    client.print("Connection: close\r\n\r\n");

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }

    String response = client.readString();
    Serial.println("Response: " + response);

  } else {
    Serial.println("Connection FAILED");
  }

  client.stop();
  }

  delay(20000); 
}
