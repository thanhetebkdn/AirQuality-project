struct Button {
	const uint8_t PIN;
	uint32_t numberKeyPresses;
	bool pressed;
  uint32_t lastDebounce;
  const uint32_t debounceDelay = 200;
};

Button button1 = {25, 0, false, 0};

void IRAM_ATTR isr() {
  uint32_t timeCurrent =  millis();

  if(timeCurrent - button1.lastDebounce > button1.debounceDelay) {
	button1.numberKeyPresses++;
	button1.pressed = true;
  button1.lastDebounce = timeCurrent;   
  }
}

void setup() {
	Serial.begin(115200);
	pinMode(button1.PIN, INPUT_PULLUP);
	attachInterrupt(button1.PIN, isr, FALLING);
}

void loop() {
	if (button1.pressed) {
		Serial.printf("Button has been pressed %u times\n", button1.numberKeyPresses);
		button1.pressed = false;
	}
}