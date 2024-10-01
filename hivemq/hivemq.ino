#include <WiFi.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define DHTPIN 15               
#define DHTTYPE DHT11           
DHT dht(DHTPIN, DHTTYPE);


const char* ssid = "Xom Tro 1";      
const char* password = "123conga";   

const char* mqtt_server = "9ff7247dfda349efa3ffa3aeba591ebe.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "iot_airquality"; 
const char* mqtt_password = "15032002Az"; 
//--------------------------------------------------
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientID =  "ESP32Client-";   // Đổi tên client ID để phù hợp với ESP32
    clientID += String(random(0xffff), HEX);
    if (client.connect(clientID.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("esp32/client");  
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage += (char)payload[i];
  Serial.println("Message arrived [" + String(topic) + "] " + incommingMessage);
}

void publishMessage(const char* topic, String payload, boolean retained) {
  if (client.publish(topic, payload.c_str(), true))
    Serial.println("Message published [" + String(topic) + "]: " + payload);
}

void setup() {
  Serial.begin(115200);          
  while (!Serial) delay(1);

  dht.begin();  

  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

unsigned long timeUpdata = millis();
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Đọc DHT11
  if (millis() - timeUpdata > 5000) {
    float h = dht.readHumidity();    
    float t = dht.readTemperature();  

  
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      Serial.print("Humidity: ");
      Serial.println(h);
      Serial.print("Temperature: ");
      Serial.println(t);

      DynamicJsonDocument doc(1024);
      doc["humidity"] = h;
      doc["temperature"] = t;
      char mqtt_message[128];
      serializeJson(doc, mqtt_message);
      publishMessage("esp32/dht11", mqtt_message, true);
    }

    timeUpdata = millis();
  }
}
