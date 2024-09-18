#include <DHTesp.h>
#include <WiFi.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>





/*-----------Define pinLED-----------*/
#define pinDht 5
DHTesp dhtSensor;

#define WIFI_AP "Xom tro 1"
#define WIFI_PASS "conga123"

#define TB_SERVER "thingsboard.cloud"
#define TOKEN "dqkGbUCQZj4D8RDSMtLz"

constexpr uint16_t MAX_MESSAGE_SIZE = 128U;

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.begin(WIFI_AP, WIFI_PASS, 6);
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi.");
  } else {
    Serial.println("\nConnected to WiFi");
  }
}

void connectToThingsBoard() {
  if (!tb.connected()) {
    Serial.println("Connecting to ThingsBoard server");
    
    if (!tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Failed to connect to ThingsBoard");
    } else {
      Serial.println("Connected to ThingsBoard");
    }
  }
}

void sendDataToThingsBoard(float temp, int hum) {
  String jsonData = "{\"temperature\":" + String(temp) + ", \"humidity\":" + String(hum) + "}";
  tb.sendTelemetryJson(jsonData.c_str());
  Serial.println("Data sent");
}

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(pinDht, DHTesp::DHT11);  // Changed to DHT11
  connectToWiFi();
  connectToThingsBoard();
}

void loop() {
  connectToWiFi();

  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  // Check if data is valid
  if (isnan(data.temperature) || isnan(data.humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  float temp = data.temperature;
  int hum = data.humidity;

  Serial.print("Temperature: ");
  Serial.println(temp);
  Serial.print("Humidity: ");
  Serial.println(hum);

  if (!tb.connected()) {
    connectToThingsBoard();
  }

  sendDataToThingsBoard(temp, hum);

  delay(3000);

  tb.loop();
}
