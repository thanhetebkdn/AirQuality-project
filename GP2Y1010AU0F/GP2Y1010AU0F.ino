int measurePin = 25;  // Chân đo cho ESP32
int ledPower = 26;    // Chân nguồn cho LED

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;

float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

void setup() {
  Serial.begin(9600);
  pinMode(ledPower, OUTPUT);
}

void loop() {
  digitalWrite(ledPower, LOW); // Bật IR LED
  delayMicroseconds(samplingTime); // Delay 0.28ms
  
  voMeasured = analogRead(measurePin); // Đọc giá trị ADC V0
  delayMicroseconds(deltaTime); // Delay 0.04ms
  digitalWrite(ledPower, HIGH); // Tắt LED
  delayMicroseconds(sleepTime); // Delay 9.68ms

  // Tính điện áp từ giá trị ADC
  calcVoltage = voMeasured * (3.3 / 4095.0); // Chuyển đổi giá trị ADC sang điện áp cho ESP32
  
  // Tính mật độ bụi dựa trên điện áp đo được
  if (calcVoltage < 0.1) {
    dustDensity = 0; // Loại bỏ nhiễu khi điện áp thấp
  } else {
    dustDensity = 0.17 * calcVoltage - 0.1; // Áp dụng phương trình tính mật độ bụi
  }

  // In kết quả ra Serial Monitor
  Serial.print("Raw Signal Value (0-4095): ");
  Serial.print(voMeasured);
  Serial.print(" - Voltage: ");
  Serial.print(calcVoltage, 2);
  Serial.print(" V - Dust Density: ");
  Serial.print(dustDensity, 2);
  Serial.println(" mg/m³");

  delay(1000); // Đợi 1 giây trước khi đo lại
}
