#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 

/*-----------Include Oled-----------*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*-----------Define Oled-----------*/
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Thông tin kết nối WiFi
const char* ssid = "Giangvien";
const char* password = "dhbk@2024";

/*-------------Setup Thingboard-------------*/
const char* mqtt_server = "thingsboard.cloud";
const char* access_token = "6SkHWZqc2YsZizNiavWQ";

const int ledPin = 15;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting......! ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected....!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  /*-------------Convert payload to string-------------*/
  String payloadStr = "";
  for (int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  
  Serial.println(payloadStr);

  /*-------------Create a document JSON-------------*/
  DynamicJsonDocument doc(1024);

  
  /*-------------Analys document JSON-------------*/
  DeserializationError error = deserializeJson(doc, payloadStr);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  /*-------------Extract value from document JSON-------------*/
  String method = doc["method"];
  bool params = doc["params"];

  /*-------------Controll LED base on method and params-------------*/
  if (method == "setValue") {
    if (params) {
      digitalWrite(ledPin, HIGH);  
      Serial.println("LED ON");
      display.clearDisplay();
      display.setCursor(0, 10);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.println("LED Status: ON");
    } else {
      digitalWrite(ledPin, LOW);   
      Serial.println("LED OFF");
      display.clearDisplay();
      display.setCursor(0, 10);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.println("LED Status: OFF");
    }
    display.display();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    /*-------------Connect from Thingboard-------------*/
    if (client.connect("ESP32_Client", access_token, NULL)) {
      Serial.println("Connected\n");
      
      /*-------------Subscribe attribute from Thingboard-------------*/
      client.subscribe("v1/devices/me/rpc/request/+");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println("Try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Update date");
  display.display(); 
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
