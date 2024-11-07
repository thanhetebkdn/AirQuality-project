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
#define MEASURE_PIN 25   
#define LED_POWER_PIN 26
#define LED_PIN 13
#define BUZZER_PIN 19
#define TEMP_THRESHOLD 35.0
#define HUMIDITY_THRESHOLD 45.0
#define AIR_QUALITY_THRESHOLD 400
#define BUZZER_TEMP_THRESHOLD 50

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
int samplingTime = 280;  
int deltaTime = 40;      
int sleepTime = 9680;   

float voMeasured = 0;   
float calcVoltage = 0;  
float dust_density = 0;

QueueHandle_t sensorDataQueue;

TaskHandle_t dhtTaskHandle;
TaskHandle_t mqTaskHandle;
TaskHandle_t warnTaskHandle;
TaskHandle_t oledTaskHandle;
TaskHandle_t dustTaskHandle;

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
                    ", \"dust_density\":" + String(dust, 2) + "}";
  tb.sendTelemetryJson(jsonData.c_str());
  Serial.println("Data sent to ThingsBoard");
}

void dhtTask(void *pvParameters)
{
  while (1)
  {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (!isnan(humidity) && !isnan(temperature))
    {

      float sensorData[4] = {temperature, humidity, air_quality_ppm, dust_density};

      if (xQueueSend(sensorDataQueue, (void *)sensorData, portMAX_DELAY) == pdPASS)
      {
        Serial.println("Data sent to Queue successfully in DHT Task!");
        Serial.print("Sent Data -> Temperature: ");
        Serial.print(temperature, 2);
        Serial.print(" C, Humidity: ");
        Serial.print(humidity, 2);
        Serial.println(" %");
        Serial.println("---------------------------------------");
      }
      else
      {
        Serial.println("Failed to send data to Queue in DHT Task!");
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // 4 seconds delay
  }
}

void mqTask(void *pvParameters)
{
  while (1)
  {
    int sensorValue = analogRead(MQ135_PIN);
    air_quality_ppm = map(sensorValue, 0, 4095, 0, 1000);
    float sensorData[4] = {temperature, humidity, air_quality_ppm, dust_density};

    if (xQueueSend(sensorDataQueue, (void *)sensorData, portMAX_DELAY) == pdPASS)
    {
      Serial.println("Data sent to Queue successfully in MQ Task!");
      Serial.print("Sent Data -> Air Quality: ");
      Serial.print(air_quality_ppm, 2);
      Serial.println(" PPM");
      Serial.println("---------------------------------------");
    }
    else
    {
      Serial.println("Failed to send data to Queue in MQ Task!");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // 4 seconds delay
  }
}

void dustTask(void *pvParameters) {
  while (1) {
    // Bật IR LED
    digitalWrite(LED_POWER_PIN, LOW); 
    delayMicroseconds(samplingTime); // Delay 0.28ms

    voMeasured = analogRead(MEASURE_PIN); // Đọc giá trị ADC V0
    delayMicroseconds(deltaTime); // Delay 0.04ms
    digitalWrite(LED_POWER_PIN, HIGH); // Tắt LED
    delayMicroseconds(sleepTime); // Delay 9.68ms

    // Tính điện áp từ giá trị ADC
    calcVoltage = voMeasured * (3.3 / 4095.0); // Chuyển đổi giá trị ADC sang điện áp cho ESP32

    // Tính mật độ bụi dựa trên điện áp đo được
    if (calcVoltage < 0.1) {
      dust_density = 0; // Loại bỏ nhiễu khi điện áp thấp
    } else {
      dust_density = 0.17 * calcVoltage - 0.1; // Áp dụng phương trình tính mật độ bụi
    }

    // Lưu dữ liệu vào mảng để gửi vào Queue
    float sensorData[4] = {temperature, humidity, air_quality_ppm, dust_density};
    if (xQueueSend(sensorDataQueue, (void *)sensorData, portMAX_DELAY) == pdPASS) {
      Serial.println("Data sent to Queue successfully in Dust Task!");
      Serial.print("Sent Data -> Dust Density: ");
      Serial.print(dust_density, 2);
      Serial.println(" µg/m³");
    } else {
      Serial.println("Failed to send data to Queue in Dust Task!");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay
  }
}



void warnTask(void *pvParameters)
{
  static unsigned long lastBlinkTime = 0;
  static bool ledState = LOW;
  unsigned long currentMillis;

  while (1)
  {
    if (temperature > TEMP_THRESHOLD || humidity < HUMIDITY_THRESHOLD)
    {
      currentMillis = millis();
      if (currentMillis - lastBlinkTime >= 500)
      {
        lastBlinkTime = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        digitalWrite(BUZZER_PIN, LOW);
      }
    }
    else
    {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, HIGH);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void oledTask(void *pvParameters)
{
  while (1)
  {
    float receivedData[4];

    // Kiểm tra nếu nhận dữ liệu từ Queue thành công
    if (xQueueReceive(sensorDataQueue, (void *)receivedData, portMAX_DELAY) == pdTRUE)
    {
      // Gán giá trị từ Queue vào các biến tương ứng
      temperature = receivedData[0];
      humidity = receivedData[1];
      air_quality_ppm = receivedData[2];
      dust_density = receivedData[3];

      // In giá trị nhận được từ Queue trong Serial Monitor
      Serial.println("Received Data from Queue:");
      Serial.print("Temperature: ");
      Serial.println(temperature, 2);
      Serial.print("Humidity: ");
      Serial.println(humidity, 2);
      Serial.print("Air Quality: ");
      Serial.println(air_quality_ppm, 2);
      Serial.print("Dust Density: ");
      Serial.println(dust_density); // Hiển thị giá trị bụi ở µg/m³
      Serial.println("-----------------------------------");

      display.clearDisplay(); // Xóa màn hình trước khi hiển thị mới

      // Kiểm tra điều kiện cảnh báo và hiển thị
      if (temperature > TEMP_THRESHOLD || humidity < HUMIDITY_THRESHOLD)
      {
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.setTextColor(WHITE);
        display.println("WARNING");
      }
      else
      {
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("Temperature: ");
        display.print(temperature, 2);
        display.println(" C");

        display.setCursor(0, 16);
        display.print("Humidity: ");
        display.print(humidity, 2);
        display.println(" %");

        display.setCursor(0, 32);
        display.print("A-Quality: ");
        display.print(air_quality_ppm, 2);
        display.println(" PPM");

        display.setCursor(0, 48);
        display.print("Dust: ");
        display.print(dust_density ); // Hiển thị giá trị bụi ở µg/m³
        display.println(" µg/m³");
      }

      display.display(); // Hiển thị toàn bộ dữ liệu một lần
    }
    else
    {
      Serial.println("Failed to receive data from Queue in OLED Task!");
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS); // Delay 4 giây để dễ dàng quan sát
  }
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH);
  pinMode(LED_POWER_PIN, OUTPUT);
  // digitalWrite(LED_POWER_PIN, HIGH);

  connectToWiFi();
  connectToThingsBoard();

  sensorDataQueue = xQueueCreate(10, sizeof(float) * 4);

  xTaskCreate(dhtTask, "DHT_Task", 2048, NULL, 1, &dhtTaskHandle);
  xTaskCreate(mqTask, "MQ_Task", 2048, NULL, 1, &mqTaskHandle);
  xTaskCreate(dustTask, "Dust_Task", 2048, NULL, 1, &dustTaskHandle);
  xTaskCreate(warnTask, "LED_Task", 2048, NULL, 2, &warnTaskHandle);
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
