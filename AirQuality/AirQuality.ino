#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include <WiFi.h>

#define DHTPIN 15
#define DHTTYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MQ135_PIN 36
#define MEASUREPIN 25
#define LEDPOWER 26

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;

float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

#define LED_PIN 13
#define BUZZER_PIN 19
#define TEMP_THRESHOLD 35.0
#define HUMIDITY_THRESHOLD 45.0
#define AIR_QUALITY_THRESHOLD 400
#define BUZZER_TEMP_THRESHOLD 50
#define PPM_THRESHOLD 400
#define DUST_THRESHOLD 12
#define BUTTON_PIN 23

#define WIFI_AP "Xom Tro 1"
#define WIFI_PASS "123conga"
#define TB_SERVER "thingsboard.cloud"
#define TOKEN "yYaO0sS1EwIGCwa00aXM"

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, 128);

float temperature = 0.0;
float humidity = 0.0;
float air_quality_ppm = 0.0;
float dust_density = 0.0;

TaskHandle_t dhtTaskHandle;
TaskHandle_t mqTaskHandle;
TaskHandle_t warnTaskHandle;
TaskHandle_t oledTaskHandle;
TaskHandle_t dustTaskHandle;

struct SensorData
{
  float temperature;
  float humidity;
  float air_quality_ppm;
  float dust_density;
};

QueueHandle_t sensorQueue;

void sendDataToQueue(float temp, float hum, float airQuality, float dust)
{
  SensorData data = {temp, hum, airQuality, dust};
  if (xQueueSend(sensorQueue, &data, portMAX_DELAY) == pdPASS)
  {
    Serial.println("Data sent to Queue successfully");
  }
  else
  {
    Serial.println("Failed to send data to Queue");
  }
}

void connectToWiFi()
{
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    WiFi.begin(WIFI_AP, WIFI_PASS);
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nConnected to WiFi");
  }
  else
  {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

void connectToThingsBoard()
{
  if (!tb.connected())
  {
    Serial.println("Connecting to ThingsBoard server");
    if (tb.connect(TB_SERVER, TOKEN))
    {
      Serial.println("Connected to ThingsBoard");
    }
    else
    {
      Serial.println("Failed to connect to ThingsBoard");
    }
  }
}

void sendDataToThingsBoard(float temp, float hum, float air_quality, float dust)
{
  String jsonData = "{\"temperature\":" + String(temp, 2) +
                    ", \"humidity\":" + String(hum, 2) +
                    ", \"air_quality\":" + String(air_quality, 2) +
                    ", \"dustDensity\":" + String(dust, 2) + "}";
  tb.sendTelemetryJson(jsonData.c_str());
  Serial.println("Data sent to ThingsBoard");
}

void dhtTask(void *pvParameters)
{
  while (1)
  {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (!isnan(temp) && !isnan(hum))
    {
      temperature = temp;
      humidity = hum;
      Serial.print("DHT Data: ");
      Serial.print("Temp: ");
      Serial.print(temperature);
      Serial.print(" °C, Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");
      sendDataToQueue(temperature, humidity,
                      air_quality_ppm, dust_density);
    }
    else
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void mqTask(void *pvParameters)
{
  while (1)
  {
    int sensorValue = analogRead(MQ135_PIN);
    air_quality_ppm = map(sensorValue, 0, 4095, 0, 1000);
    Serial.print("MQ-135 PPM: ");
    Serial.println(air_quality_ppm);

    // In giá trị trước khi gửi vào Queue
    Serial.print("Sending to Queue: Temp: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Air Quality: ");
    Serial.print(air_quality_ppm);
    Serial.print(" PPM, Dust: ");
    Serial.print(dust_density);
    Serial.println(" ug/m3");

    sendDataToQueue(temperature, humidity, air_quality_ppm, dust_density); // Gửi giá trị vào Queue
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task đọc dữ liệu Dust Sensor
void dustTask(void *pvParameters)
{
  while (1)
  {
    digitalWrite(LEDPOWER, LOW);
    delayMicroseconds(samplingTime);
    float voMeasured = analogRead(MEASUREPIN);
    delayMicroseconds(deltaTime);
    digitalWrite(LEDPOWER, HIGH);
    delayMicroseconds(sleepTime);

    float calcVoltage = voMeasured * (3.3 / 4095.0);
    dust_density = 0.17 * calcVoltage - 0.05; // Điều chỉnh hệ số và bù trừ

    // Đảm bảo giá trị luôn lớn hơn 0
    if (dust_density < 0.01)
    {
      dust_density = 0.05; // Giá trị tối thiểu
    }

    Serial.print("Dust Density: ");
    Serial.print(dust_density);
    Serial.println(" mg/m3");

    // In giá trị trước khi gửi vào Queue
    Serial.print("Sending to Queue: Temp: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Air Quality: ");
    Serial.print(air_quality_ppm);
    Serial.print(" PPM, Dust: ");
    Serial.print(dust_density);
    Serial.println(" ug/m3");

    sendDataToQueue(temperature, humidity, air_quality_ppm, dust_density); // Gửi giá trị vào Queue
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void warnTask(void *pvParameters)
{
  SensorData sensorData; // Cấu trúc chứa dữ liệu cảm biến
  static unsigned long lastBlinkTime = 0;
  static bool ledState = LOW;
  unsigned long currentMillis;

  while (1)
  {
    // Đọc từ Queue
    if (xQueueReceive(sensorQueue, &sensorData, portMAX_DELAY) == pdPASS)
    {
      float temp = sensorData.temperature;
      float hum = sensorData.humidity;
      float air = sensorData.air_quality_ppm;
      float dust = sensorData.dust_density;

      // In giá trị khi lấy từ Queue
      Serial.print("Received from Queue: Temp: ");
      Serial.print(temp);
      Serial.print(" °C, Humidity: ");
      Serial.print(hum);
      Serial.print(" %, Air Quality: ");
      Serial.print(sensorData.air_quality_ppm);
      Serial.print(" PPM, Dust: ");
      Serial.print(sensorData.dust_density);
      Serial.println(" ug/m3");

      // Kiểm tra điều kiện cảnh báo
      if (temp > TEMP_THRESHOLD || hum < HUMIDITY_THRESHOLD || air > PPM_THRESHOLD || dust > DUST_THRESHOLD)
      {
        currentMillis = millis();
        if (currentMillis - lastBlinkTime >= 500)
        {
          lastBlinkTime = currentMillis;
          ledState = !ledState;            // Thay đổi trạng thái LED
          digitalWrite(LED_PIN, ledState); // Bật/tắt LED
        }
        digitalWrite(BUZZER_PIN, LOW); // Bật còi
      }
      else
      {
        digitalWrite(LED_PIN, LOW);     // Tắt LED
        digitalWrite(BUZZER_PIN, HIGH); // Tắt còi
      }
      sendDataToThingsBoard(sensorData.temperature, sensorData.humidity, sensorData.air_quality_ppm, sensorData.dust_density);
    }

    vTaskDelay(500 / portTICK_PERIOD_MS); // Điều chỉnh thời gian trễ hợp lý
  }
}

void oledTask(void *pvParameters)
{
  SensorData receivedData;

  while (1)
  {
    if (xQueueReceive(sensorQueue, &receivedData, portMAX_DELAY) == pdPASS)
    {
      display.clearDisplay();
      display.setCursor(0, 0);

      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.print("Temp: ");
      display.print(receivedData.temperature, 2);
      display.println(" C");

      display.setCursor(0, 16);
      display.print("Humidity: ");
      display.print(receivedData.humidity, 2);
      display.println(" %");

      display.setCursor(0, 32);
      display.print("Air: ");
      display.print(receivedData.air_quality_ppm, 2);
      display.println(" PPM");

      display.setCursor(0, 48);
      display.print("Dust: ");
      display.print(receivedData.dust_density, 2);
      display.println(" ug/m3");

      display.display();
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED init failed");
    while (1)
      ;
  }
  display.clearDisplay();

  sensorQueue = xQueueCreate(5, sizeof(SensorData));
  if (sensorQueue == NULL)
  {
    Serial.println("Failed to create Queue");
    while (1)
      ;
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH);
  pinMode(LEDPOWER, OUTPUT);

  connectToWiFi();
  connectToThingsBoard();

  xTaskCreate(dhtTask, "DHT_Task", 2048, NULL, 1, &dhtTaskHandle);
  xTaskCreate(mqTask, "MQ_Task", 2048, NULL, 1, &mqTaskHandle);
  xTaskCreate(dustTask, "Dust_Task", 1024, NULL, 1, &dustTaskHandle);
  xTaskCreate(warnTask, "Warn_Task", 4096, NULL, 2, &warnTaskHandle);
  xTaskCreate(oledTask, "Display_Task", 2048, NULL, 2, &oledTaskHandle);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
    connectToWiFi();
  if (!tb.connected())
    connectToThingsBoard();
  tb.loop();
}
