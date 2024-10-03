#include <Arduino.h>
#include <freertos/FreeRTOS.h>

int dustPin = 27;
int ledPower = 25;
int delayTime = 280;
int delayTime2 = 40;
float offTime = 9680;

void setup()
{
  Serial.begin(9600);
  pinMode(ledPower, OUTPUT);
  xTaskCreate(dustSensorTask, "Dust Sensor Task", 2048, NULL, 1, NULL);
}

void loop()
{
}

void dustSensorTask(void *pvParameters)
{
  float voltage = 0;
  float dustdensity = 0;
  char s[32];

  while (true)
  {
    digitalWrite(ledPower, LOW);
    delayMicroseconds(delayTime);
    int dustVal = analogRead(dustPin);
    delayMicroseconds(delayTime2);
    digitalWrite(ledPower, HIGH);
    delayMicroseconds(offTime);

    voltage = dustVal * 0.00322;

    dustdensity = (0.172 * voltage) - 0.1;

    if (dustdensity < 0)
      dustdensity = 0;
    if (dustdensity > 0.5)
      dustdensity = 0.5;

    String dataString = "";
    dataString += dtostrf(voltage, 9, 4, s);
    dataString += "V,";
    dataString += dtostrf(dustdensity * 1000.0, 5, 2, s);
    dataString += "ug/m3";

    Serial.println(dataString);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
