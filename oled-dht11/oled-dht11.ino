#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>

// DHT sensor settings
#define DHTPIN 15     // Pin where the DHT11 is connected
#define DHTTYPE DHT11 // DHT11 sensor type
DHT dht(DHTPIN, DHTTYPE);

// OLED display settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup()
{
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println(F("DHT-11 and OLED test!"));

  // Initialize the DHT sensor
  dht.begin();

  // Initialize OLED display
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
}

void loop()
{
  // Delay between readings
  delay(2000);

  // Reading humidity and temperature
  float h = dht.readHumidity();
  float t = dht.readTemperature();     // Celsius
  float f = dht.readTemperature(true); // Fahrenheit

  // Check if any reading failed
  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Print the values on Serial Monitor
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));

  // Display the readings on the OLED
  display.clearDisplay();
  display.setCursor(0, 20);

  display.print("Temp: ");
  display.print(t);
  display.println(" C");

  display.print("Hump: ");
  display.print(h);
  display.println("%");

  display.display();
}
