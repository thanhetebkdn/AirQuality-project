
#include <WiFi.h>
#include "DHT.h"

const char *ssid = "Xom Tro 1";
const char *password = "123conga";

WiFiServer server(80);

String header;

String outputLED_1 = "OFF";
String outputLED_2 = "OFF";

const int output_1 = 33;
const int output_2 = 27;

#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

unsigned long currentTime = millis();

unsigned long previousTime = 0;

const long timeoutTime = 2000;

void setup()
{
  Serial.begin(115200);
  pinMode(output_1, OUTPUT);
  pinMode(output_2, OUTPUT);
  digitalWrite(output_1, LOW);
  digitalWrite(output_2, LOW);

  dht.begin();

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop()
{
  WiFiClient client = server.available();

  if (client)
  {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    {
      currentTime = millis();
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n')
        {
          if (currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Điều khiển GPIO
            if (header.indexOf("GET /26/on") >= 0)
            {
              Serial.println("GPIO 26 on");
              outputLED_1 = "on";
              digitalWrite(output_1, HIGH);
            }
            else if (header.indexOf("GET /26/off") >= 0)
            {
              Serial.println("GPIO 26 off");
              outputLED_1 = "off";
              digitalWrite(output_1, LOW);
            }
            else if (header.indexOf("GET /27/on") >= 0)
            {
              Serial.println("GPIO 27 on");
              outputLED_2 = "on";
              digitalWrite(output_2, HIGH);
            }
            else if (header.indexOf("GET /27/off") >= 0)
            {
              Serial.println("GPIO 27 off");
              outputLED_2 = "off";
              digitalWrite(output_2, LOW);
            }

            // Hiển thị trang web
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>");
            client.println("html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("body { background-color: #f7f7f7; margin: 0; padding: 0;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 20px 40px;");
            client.println("text-decoration: none; font-size: 24px; margin: 10px; cursor: pointer; border-radius: 12px; transition: background-color 0.3s, transform 0.3s;}");
            client.println(".button2 {background-color: #555555;}");
            client.println(".button:hover { background-color: #45a049; transform: scale(1.05); }");
            client.println(".button2:hover { background-color: #333333; transform: scale(1.05); }");
            client.println("</style>");
            client.println("<script>");
            client.println("setInterval(function() {");
            client.println("fetch('/dht').then(response => response.json()).then(data => {");
            client.println("document.getElementById('humidity').innerHTML = 'Humidity: ' + data.humidity + ' %';");
            client.println("document.getElementById('temperature').innerHTML = 'Temperature: ' + data.temperature + ' C';");
            client.println("});");
            client.println("}, 2000);");
            client.println("</script></head>");

            client.println("<body><h1>ESP32 Web Server</h1>");

            client.println("<p id='temperature' style=\"font-size:24px; color:red;\">Temperature: " + String(dht.readTemperature()) + " C</p>");
            client.println("<p id='humidity' style=\"font-size:24px; color:blue;\">Humidity: " + String(dht.readHumidity()) + " %</p>");

            client.println("<p>GPIO 26 - State " + outputLED_1 + "</p>");
            if (outputLED_1 == "off")
            {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            }
            else
            {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            }

            // Hiển thị trạng thái GPIO 27
            client.println("<p>GPIO 27 - State " + outputLED_2 + "</p>");
            if (outputLED_2 == "off")
            {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            }
            else
            {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            client.println();
            break;
          }
          else
          {
            currentLine = "";
          }
        }
        else if (c != '\r')
        {
          currentLine += c;
        }
      }
    }

    if (header.indexOf("GET /dht") >= 0)
    {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      String jsonResponse = String("{\"humidity\":") + h + ",\"temperature\":" + t + "}";
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type: application/json");
      client.println("Connection: close");
      client.println();
      client.println(jsonResponse);
      delay(1);
    }

    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
