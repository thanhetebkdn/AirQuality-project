#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include <WiFi.h>

/*------------- Define DHT-11-------------*/
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

/*------------- Define OLED-------------*/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/*------------- Define MQ-135-------------*/
#define MQ135_PIN 36

/*------------- Define LED and Buzzer Pins -------------*/
#define LED_PIN 4
#define BUZZER_PIN 16

/*------------- Define thresholds -------------*/
#define TEMP_THRESHOLD 30.0
#define HUMIDITY_THRESHOLD 70.0
#define AIR_QUALITY_THRESHOLD 500
#define BUZZER_TEMP_THRESHOLD 30.0

/*------------- WiFi and ThingsBoard credentials -------------*/
#define WIFI_AP "Xom Tro 1"
#define WIFI_PASS "123conga"
#define TB_SERVER "thingsboard.cloud"
#define TOKEN "yYaO0sS1EwIGCwa00aXM"  

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, 128);

/*------------- Global Variables -------------*/
float temperature = 0.0, humidity = 0.0, air_quality_ppm = 0.0;

/*------------- Task Handles -------------*/
TaskHandle_t dhtTaskHandle;
TaskHandle_t mqTaskHandle;
TaskHandle_t ledTaskHandle;
TaskHandle_t buzzerTaskHandle;

/*------------- Functions for WiFi and ThingsBoard -------------*/
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

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nFailed to connect to WiFi.");
  }
  else
  {
    Serial.println("\nConnected to WiFi");
  }
}

void connectToThingsBoard()
{
  if (!tb.connected())
  {
    Serial.println("Connecting to ThingsBoard server");

    if (!tb.connect(TB_SERVER, TOKEN))
    {
      Serial.println("Failed to connect to ThingsBoard");
    }
    else
    {
      Serial.println("Connected to ThingsBoard");
    }
  }
}

void sendDataToThingsBoard(float temp, float hum, float air_quality)
{
  String jsonData = "{\"temperature\":" + String(temp) + ", \"humidity\":" + String(hum) + ", \"air_quality\":" + String(air_quality) + "}";
  tb.sendTelemetryJson(jsonData.c_str());
  Serial.println("Data sent to ThingsBoard");
}

void setup()
{
  /*------------- Initialize Serial Monitor-------------*/
  Serial.begin(115200);
  Serial.println(F("DHT-11, OLED, Buzzer, and ThingsBoard test!"));

  /*------------- Initialize DHT-11-------------*/
  dht.begin();

  /*------------- Initialize OLED-------------*/
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
    for (;;)
      ;
  }
  display.setFont(&FreeSerif9pt7b);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  /*------------- Initialize MQ-135-------------*/
  Serial.println("MQ-135 sensor is initialized");

  /*------------- Initialize LED and Buzzer Pins -------------*/
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  /*------------- Connect to WiFi and ThingsBoard -------------*/
  connectToWiFi();
  connectToThingsBoard();

  /*------------- Create FreeRTOS tasks -------------*/
  xTaskCreatePinnedToCore(dhtTask, "DHT_Task", 10000, NULL, 1, &dhtTaskHandle, 0);
  xTaskCreatePinnedToCore(mqTask, "MQ_Task", 10000, NULL, 1, &mqTaskHandle, 0);
  xTaskCreatePinnedToCore(ledTask, "LED_Task", 10000, NULL, 1, &ledTaskHandle, 1);
  xTaskCreatePinnedToCore(buzzerTask, "Buzzer_Task", 10000, NULL, 1, &buzzerTaskHandle, 1);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
  if (!tb.connected())
  {
    connectToThingsBoard();
  }
  tb.loop();
}

/*------------- Task to handle DHT sensor readings -------------*/
void dhtTask(void *pvParameters)
{
  while (1)
  {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature))
    {
      Serial.println(F("Failed to read from DHT sensor!"));
    }
    else
    {
      Serial.print(F("Humidity: "));
      Serial.print(humidity);
      Serial.print(F("%  Temperature: "));
      Serial.print(temperature);
      Serial.println(F("°C"));

      // Send data to ThingsBoard
      sendDataToThingsBoard(temperature, humidity, air_quality_ppm);

      // Update OLED Display
      display.clearDisplay();
      display.setCursor(0, 16);
      display.print("Temp: ");
      display.print(temperature);
      display.println(" C");
      display.setCursor(0, 32);
      display.print("Hum: ");
      display.print(humidity);
      display.println(" %");
      display.setCursor(0, 48);
      display.print("PM: ");
      display.print(air_quality_ppm);
      display.println(" PPM");
      display.display();
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

/*------------- Task to handle MQ-135 sensor readings -------------*/
void mqTask(void *pvParameters)
{
  while (1)
  {
    int sensorValue = analogRead(MQ135_PIN);
    air_quality_ppm = map(sensorValue, 0, 4095, 0, 1000);

    Serial.print("Estimated Air Quality: ");
    Serial.print(air_quality_ppm);
    Serial.println(" PPM");

    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

/*------------- Task to handle LED control -------------*/
void ledTask(void *pvParameters)
{
  static unsigned long lastBlinkTime = 0;
  static bool ledState = LOW;
  unsigned long currentMillis;

  while (1)
  {
    if (temperature > TEMP_THRESHOLD || humidity > HUMIDITY_THRESHOLD || air_quality_ppm > AIR_QUALITY_THRESHOLD)
    {
      currentMillis = millis();
      if (currentMillis - lastBlinkTime >= 500)
      {
        lastBlinkTime = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
      }
    }
    else
    {
      digitalWrite(LED_PIN, LOW);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/*------------- Task to handle Buzzer control -------------*/
void buzzerTask(void *pvParameters)
{
  while (1)
  {
    if (temperature > BUZZER_TEMP_THRESHOLD)
    {
      digitalWrite(BUZZER_PIN, HIGH);
    }
    else
    {
      digitalWrite(BUZZER_PIN, LOW);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
