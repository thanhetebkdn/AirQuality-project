#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include <WiFi.h>

// WiFi và ThingsBoard
#define WIFI_AP "Xom Tro 1"
#define WIFI_PASS "123conga"
#define TB_SERVER "thingsboard.cloud"
#define TOKEN "yYaO0sS1EwIGCwa00aXM"

// Định nghĩa cảm biến và ngưỡng
#define DHTPIN 15
#define DHTTYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MQ135_PIN 36
#define MEASURE_PIN 25
#define LED_POWER_PIN 26
#define LED_PIN 13
#define BUZZER_PIN 19
#define TEMP_THRESHOLD 35.0
#define HUMIDITY_THRESHOLD 45.0

// Các biến toàn cục và cấu trúc dữ liệu
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, 128);

typedef struct {
  float temperature;
  float humidity;
  float air_quality_ppm;
  float dust_density;
} SensorData;

SensorData currentSensorData = {0.0, 0.0, 0.0, 0.0};
QueueHandle_t sensorDataQueue;

TaskHandle_t dhtTaskHandle;
TaskHandle_t mqTaskHandle;
TaskHandle_t dustTaskHandle;
TaskHandle_t oledTaskHandle;
TaskHandle_t warnTaskHandle;

// Kết nối WiFi
void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.begin(WIFI_AP, WIFI_PASS);
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

// Kết nối ThingsBoard
void connectToThingsBoard() {
  if (!tb.connected()) {
    Serial.println("Connecting to ThingsBoard server...");
    if (tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Connected to ThingsBoard");
    } else {
      Serial.println("Failed to connect to ThingsBoard");
    }
  }
}

// Gửi dữ liệu lên ThingsBoard
void sendDataToThingsBoard() {
  tb.sendTelemetryData("temperature", String(currentSensorData.temperature).c_str());
  tb.sendTelemetryData("humidity", String(currentSensorData.humidity).c_str());
  tb.sendTelemetryData("air_quality", String(currentSensorData.air_quality_ppm).c_str());
  tb.sendTelemetryData("dust_density", String(currentSensorData.dust_density).c_str());
  Serial.println("Data sent to ThingsBoard");
}


// Task đọc DHT11
void dhtTask(void *pvParameters) {
  while (1) {
    SensorData data;
    data.temperature = dht.readTemperature();
    data.humidity = dht.readHumidity();

    if (!isnan(data.temperature) && !isnan(data.humidity)) {
      if (xQueueSend(sensorDataQueue, &data, portMAX_DELAY) == pdPASS) {
        Serial.println("DHT data sent to queue");
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task đọc MQ135
void mqTask(void *pvParameters) {
  while (1) {
    SensorData data;
    data.air_quality_ppm = map(analogRead(MQ135_PIN), 0, 4095, 0, 1000);

    if (xQueueSend(sensorDataQueue, &data, portMAX_DELAY) == pdPASS) {
      Serial.println("MQ135 data sent to queue");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task đọc cảm biến bụi
void dustTask(void *pvParameters) {
  while (1) {
    SensorData data;
    digitalWrite(LED_POWER_PIN, LOW);
    delayMicroseconds(280);
    float voMeasured = analogRead(MEASURE_PIN);
    delayMicroseconds(40);
    digitalWrite(LED_POWER_PIN, HIGH);

    float calcVoltage = voMeasured * (3.3 / 4095.0);
    data.dust_density = (calcVoltage < 0.1) ? 0 : (0.17 * calcVoltage - 0.1);

    if (xQueueSend(sensorDataQueue, &data, portMAX_DELAY) == pdPASS) {
      Serial.println("Dust data sent to queue");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task hiển thị OLED và gửi dữ liệu lên ThingsBoard
void oledTask(void *pvParameters) {
  while (1) {
    SensorData receivedData;

    if (xQueueReceive(sensorDataQueue, &receivedData, portMAX_DELAY) == pdTRUE) {
      if (receivedData.temperature != 0.0)
        currentSensorData.temperature = receivedData.temperature;
      if (receivedData.humidity != 0.0)
        currentSensorData.humidity = receivedData.humidity;
      if (receivedData.air_quality_ppm != 0.0)
        currentSensorData.air_quality_ppm = receivedData.air_quality_ppm;
      if (receivedData.dust_density != 0.0)
        currentSensorData.dust_density = receivedData.dust_density;

      // Hiển thị dữ liệu
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.print("Temp: ");
      display.print(currentSensorData.temperature, 2);
      display.println(" C");
    display.setCursor(0, 16);

      display.print("Hum: ");
      display.print(currentSensorData.humidity, 2);
      display.println(" %");

display.setCursor(0, 32);
      display.print("Air-Q: ");
      display.print(currentSensorData.air_quality_ppm, 2);
      display.println(" PPM");

display.setCursor(0, 48);
      display.print("Dust: ");
      display.print(currentSensorData.dust_density, 2);
      display.println(" ug/m3");
      display.display();

      // Gửi dữ liệu lên ThingsBoard
      sendDataToThingsBoard();
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// Task cảnh báo bằng LED và Buzzer
void warnTask(void *pvParameters) {
  while (1) {
    if (currentSensorData.temperature > TEMP_THRESHOLD || currentSensorData.humidity < HUMIDITY_THRESHOLD) {
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, LOW); // Buzzer bật
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, HIGH); // Buzzer tắt
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  dht.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH);
  pinMode(LED_POWER_PIN, OUTPUT);

  connectToWiFi();
  connectToThingsBoard();

  sensorDataQueue = xQueueCreate(10, sizeof(SensorData));

  xTaskCreate(dhtTask, "DHT_Task", 2048, NULL, 1, &dhtTaskHandle);
  xTaskCreate(mqTask, "MQ_Task", 2048, NULL, 1, &mqTaskHandle);
  xTaskCreate(dustTask, "Dust_Task", 2048, NULL, 1, &dustTaskHandle);
  xTaskCreate(oledTask, "OLED_Task", 2048, NULL, 2, &oledTaskHandle);
  xTaskCreate(warnTask, "Warn_Task", 2048, NULL, 2, &warnTaskHandle);
}

// Loop
void loop() {
  if (WiFi.status() != WL_CONNECTED)
    connectToWiFi();
  if (!tb.connected())
    connectToThingsBoard();
  tb.loop();
}
