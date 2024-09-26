int measurePin = 18;
int ledPower = 17;
 
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
 
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;
 
void setup(){
  Serial.begin(9600);
  pinMode(ledPower,OUTPUT);
}
 
void loop(){
  digitalWrite(ledPower,LOW); // Bật IR LED
  delayMicroseconds(samplingTime);  //Delay 0.28ms
  voMeasured = analogRead(measurePin); // Đọc giá trị ADC V0
  delayMicroseconds(deltaTime); //Delay 0.04ms
  digitalWrite(ledPower,HIGH); // Tắt LED
  delayMicroseconds(sleepTime); //Delay 9.68ms
 
  // Tính điện áp từ giá trị ADC
  calcVoltage = voMeasured * (5.0 / 1024); //Điệp áp Vcc của cảm biến (5.0 hoặc 3.3)
 
  // Linear Equation http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = 0.17 * calcVoltage - 0.1;
 
 
  Serial.print("Raw Signal Value (0-1023): ");
  Serial.print(voMeasured);
  Serial.print(" - Voltage: ");
  Serial.print(calcVoltage);
  Serial.print(" - Dust Density: ");
  Serial.println(dustDensity);
  delay(1000);
}