#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

/*------------- Define GP2Y1010 Dust Sensor Pins -------------*/
#define DUST_PIN 27
#define LED_POWER_PIN 25

/*------------- Define LED and Buzzer Pins -------------*/
#define LED_PIN 33
#define BUZZER_PIN 19

/*------------- Define thresholds -------------*/
#define TEMP_THRESHOLD 40.0
#define HUMIDITY_THRESHOLD 45.0
#define AIR_QUALITY_THRESHOLD 400
#define BUZZER_TEMP_THRESHOLD 50

/*------------- WiFi and ThingsBoard credentials -------------*/
#define WIFI_AP "Xom Tro 1"
#define WIFI_PASS "123conga"
#define TB_SERVER "thingsboard.cloud"
#define TOKEN "yYaO0sS1EwIGCwa00aXM"  

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, 128);

/*------------- Define buttons -------------*/
#define BUTTON_UP_PIN 12    // Nút Lên
#define BUTTON_DOWN_PIN 13  // Nút Xuống
#define BUTTON_SELECT_PIN 14 // Nút Select
#define BUTTON_BACK_PIN 26  // Nút Back

/*------------- Global Variables -------------*/
float temperature = 0.0, humidity = 0.0, air_quality_ppm = 0.0;
float dust_density = 0.0;
int menuIndex = 0;           // Để theo dõi mục menu hiện tại
bool inRoomMenu = false;     // Kiểm tra có đang trong menu lựa chọn phòng hay không

String rooms[] = {"Room 1", "Room 2", "Room 3"};  // Danh sách các phòng
int currentRoom = 0;         // Phòng hiện tại được chọn

/*------------- Task Handles -------------*/
TaskHandle_t dhtTaskHandle;
TaskHandle_t mqTaskHandle;
TaskHandle_t ledTaskHandle;
TaskHandle_t displayTaskHandle; 
TaskHandle_t dustTaskHandle; 

/*------------- Functions for WiFi and ThingsBoard -------------*/
void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.begin(WIFI_AP, WIFI_PASS);
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

void sendDataToThingsBoard(float temp, float hum, float air_quality, float dust) {
  String jsonData = "{\"temperature\":" + String(temp, 2) + 
                     ", \"humidity\":" + String(hum, 2) + 
                     ", \"air_quality\":" + String(air_quality, 2) + 
                     ", \"dust_density\":" + String(dust, 2) + "}";
  tb.sendTelemetryJson(jsonData.c_str());
  Serial.println("Data sent to ThingsBoard");
}

void setup() {
  /*------------- Initialize Serial Monitor-------------*/
  Serial.begin(115200);
  Serial.println(F("DHT-11, OLED, Buzzer, and ThingsBoard test!"));

  /*------------- Initialize DHT-11-------------*/
  dht.begin();

  /*------------- Initialize OLED-------------*/
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  /*------------- Initialize MQ-135-------------*/
  Serial.println("MQ-135 sensor is initialized");

  /*------------- Initialize LED and Buzzer Pins -------------*/
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH);
  
  /*------------- Initialize Dust Sensor Pins -------------*/
  pinMode(LED_POWER_PIN, OUTPUT);
  digitalWrite(LED_POWER_PIN, HIGH); 

  /*------------- Initialize Buttons -------------*/
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);

  /*------------- Connect to WiFi and ThingsBoard -------------*/
  connectToWiFi();
  connectToThingsBoard();

  /*------------- Create FreeRTOS tasks -------------*/
  xTaskCreatePinnedToCore(dhtTask, "DHT_Task", 10000, NULL, 1, &dhtTaskHandle, 0);
  xTaskCreatePinnedToCore(mqTask, "MQ_Task", 10000, NULL, 1, &mqTaskHandle, 0);
  xTaskCreatePinnedToCore(ledTask, "LED_Task", 10000, NULL, 1, &ledTaskHandle, 0);
  xTaskCreatePinnedToCore(displayTask, "Display_Task", 10000, NULL, 1, &displayTaskHandle, 0);
  xTaskCreatePinnedToCore(dustTask, "Dust_Task", 10000, NULL, 1, &dustTaskHandle, 0); // New task for dust sensor
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  if (!tb.connected()) {
    connectToThingsBoard();
  }
  tb.loop();
}

/*------------- Task to handle DHT sensor readings -------------*/
void dhtTask(void *pvParameters) {
  while (1) {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println(F("Failed to read from DHT sensor!"));
    } else {
      Serial.print(F("Humidity: "));
      Serial.print(String(humidity, 2));  
      Serial.print(F("%  Temperature: "));
      Serial.print(String(temperature, 2)); 
      Serial.println(F("°C"));

      // Send data to ThingsBoard
      sendDataToThingsBoard(temperature, humidity, air_quality_ppm, dust_density);
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

/*------------- Task to handle MQ-135 sensor readings -------------*/
void mqTask(void *pvParameters) {
  while (1) {
    int sensorValue = analogRead(MQ135_PIN);
    air_quality_ppm = map(sensorValue, 0, 4095, 0, 1000);

    Serial.print("Estimated Air Quality: ");
    Serial.print(String(air_quality_ppm, 2));  
    Serial.println(" PPM");

    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

/*------------- Task to handle Dust Sensor readings -------------*/
void dustTask(void *pvParameters) {
  while (1) {
    digitalWrite(LED_POWER_PIN, LOW); 
    delayMicroseconds(280);
    int dustVal = analogRead(DUST_PIN); 
    delayMicroseconds(40);
    digitalWrite(LED_POWER_PIN, HIGH); 
    delayMicroseconds(9680);

    float voltage = dustVal * 0.0049; 
    dust_density = 0.172 * voltage - 0.1;

    Serial.print("Dust Density: ");
    Serial.print(String(dust_density * 1000.0, 2));  
    Serial.println(" µg/m³");

    vTaskDelay(2000 / portTICK_PERIOD_MS); 
  }
}

/*------------- Task to handle LED control -------------*/
void ledTask(void *pvParameters) {
  static unsigned long lastBlinkTime = 0;
  static bool ledState = LOW;
  unsigned long currentMillis;

  while (1) {
    if (temperature > TEMP_THRESHOLD || humidity < HUMIDITY_THRESHOLD || air_quality_ppm > AIR_QUALITY_THRESHOLD) {
      currentMillis = millis();
      if (currentMillis - lastBlinkTime >= 500) {
        lastBlinkTime = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        digitalWrite(BUZZER_PIN, LOW);
      }
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, HIGH);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
}

/*------------- Task to handle menu and OLED display -------------*/
void displayTask(void *pvParameters) {
  while (1) {
    if (!inRoomMenu) {
      // Hiển thị menu chính với các phòng
      display.clearDisplay();
      display.setCursor(0, 16);
      display.print("> ");
      display.println(rooms[menuIndex]); // Hiển thị tên phòng theo menuIndex
      display.display();
      
      // Xử lý nút Lên
      if (digitalRead(BUTTON_UP_PIN) == LOW) {
        menuIndex--;
        if (menuIndex < 0) menuIndex = 2;  // Quay vòng menu
        vTaskDelay(200); // Tránh trễ do nhấn nút
      }

      // Xử lý nút Xuống
      if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
        menuIndex++;
        if (menuIndex > 2) menuIndex = 0;  // Quay vòng menu
        vTaskDelay(200);
      }

      // Xử lý nút Select
      if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
        currentRoom = menuIndex;  // Chọn phòng hiện tại
        inRoomMenu = true;        // Chuyển sang chế độ hiển thị thông tin phòng
        vTaskDelay(200);
      }
    } else {
      // Hiển thị dữ liệu của phòng đã chọn
      display.clearDisplay();
      display.setCursor(0, 16);
      display.print("Room: ");
      display.println(rooms[currentRoom]);

      display.setCursor(0, 32);
      display.print("Temp: ");
      display.print(String(temperature, 2)); 
      display.println(" C");
      
      display.setCursor(0, 48);
      display.print("Hum: ");
      display.print(String(humidity, 2));  
      display.println(" %");

      display.display();
      
      // Xử lý nút Back để quay về menu chính
      if (digitalRead(BUTTON_BACK_PIN) == LOW) {
        inRoomMenu = false;  // Quay lại menu chính
        vTaskDelay(200);
      }
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
}
