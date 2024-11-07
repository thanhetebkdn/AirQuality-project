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
#define MEASUREPIN  25
#define LEDPOWER  26
  

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
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    if (!isnan(humidity) && !isnan(temperature))
    {
      Serial.print("Humidity: ");
      Serial.print(humidity, 2);
      Serial.print("%  Temperature: ");
      Serial.print(temperature, 2);
      Serial.println("°C");
      sendDataToThingsBoard(temperature, humidity, air_quality_ppm, dust_density);
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
    Serial.print("Estimated Air Quality: ");
    Serial.print(air_quality_ppm, 2);
    Serial.println(" PPM");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void dustTask(void *pvParameters)
{
  while (1)
  {

    digitalWrite(LEDPOWER, LOW); // Bật IR LED
    delayMicroseconds(samplingTime); // Delay 0.28ms
    
    voMeasured = analogRead(MEASUREPIN); // Đọc giá trị ADC V0
    delayMicroseconds(deltaTime); // Delay 0.04ms
    digitalWrite(LEDPOWER, HIGH); // Tắt LED
    delayMicroseconds(sleepTime); // Delay 9.68ms

    // Tính điện áp từ giá trị ADC
    calcVoltage = voMeasured * (3.3 / 4095.0); // Chuyển đổi giá trị ADC sang điện áp cho ESP32
    
    // Tính mật độ bụi dựa trên điện áp đo được
    if (calcVoltage < 0.1) {
      dustDensity = 0; // Loại bỏ nhiễu khi điện áp thấp
    } else {
      dustDensity = 0.17 * calcVoltage - 0.1; // Áp dụng phương trình tính mật độ bụi
    }
    Serial.print(dustDensity, 2);
    Serial.println(" mg/m³");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
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
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void oledTask(void *pvParameters)
{
  while (1)
  {
    display.clearDisplay();

    if (temperature > TEMP_THRESHOLD || humidity < HUMIDITY_THRESHOLD  )
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
      display.print("Air: ");
      display.print(air_quality_ppm, 2);
      display.println(" PPM");
      display.setCursor(0, 48);
      display.print("Dust: ");
      display.print(dust_density, 2);
      display.println(" ug/m3");
    }

    display.display();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
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
  pinMode(LEDPOWER, OUTPUT);

  connectToWiFi();
  connectToThingsBoard();

  xTaskCreate(dhtTask, "DHT_Task", 10000, NULL, 1, &dhtTaskHandle);
  xTaskCreate(mqTask, "MQ_Task", 10000, NULL, 1, &mqTaskHandle);
  xTaskCreate(dustTask, "Dust_Task", 10000, NULL, 1, &dustTaskHandle);
  xTaskCreate(warnTask, "LED_Task", 10000, NULL, 2, &warnTaskHandle);
  xTaskCreate(oledTask, "Display_Task", 10000, NULL, 2, &oledTaskHandle);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
    connectToWiFi();
  if (!tb.connected())
    connectToThingsBoard();
  tb.loop();
}
