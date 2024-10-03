#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define BUTTON_UP_PIN 14
#define BUTTON_DOWN_PIN 27
#define BUTTON_SELECT_PIN 26
#define BUTTON_BACK_PIN 25 // Define the pin for the Back button
#define DHT_PIN 4          // Define the pin for DHT-11
#define DHT_TYPE DHT11     // DHT 11

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);

// Menu variables
int currentMenu = 0;
int totalMenuItems = 3;
String mainMenuItems[] = {"Room-1", "Room-2", "Room-3"};
String room1SubMenuItems[] = {"Sensor", "Actors", "Settings"};
String sensorSubMenuItems[] = {"DHT-11", "MQ-135", "Dust-Sensor"}; // Submenu for sensors
String settingsList[] = {"Temperature", "Humidity", "Dust"};

// Track if we are in the main menu, Room-1 submenu, or sensor submenu
bool inMainMenu = true; 
bool inSensorMenu = false; // Track if we are in the sensor menu
bool inSettingsMenu = false; 

int room1MenuItems = 3;
int settingSubmenuItems = 3;

// Debounce states for buttons
bool buttonUpPressed = false;
bool buttonDownPressed = false;
bool buttonSelectPressed = false;

// Function to display the main menu
void displayMainMenu() {
    display.clearDisplay();
    display.setTextSize(1); 
    display.setTextColor(WHITE);

    // Loop through main menu items and display them
    for (int i = 0; i < totalMenuItems; i++) {
        if (i == currentMenu) {
            display.setCursor(0, i * 16); // Highlight current menu item
            display.print("> "); // Arrow indicating the selected option
        } else {
            display.setCursor(18, i * 16); // Adjust X offset for other items
        }
        display.println(mainMenuItems[i]);
    }

    // Display "HOME" at the bottom
    display.setTextSize(1);
    display.setCursor((SCREEN_WIDTH - 6 * 4) / 2, SCREEN_HEIGHT - 10); // 6 pixels per character, 4 characters for "HOME"
    display.println("HOME");
  
    display.display();
}

// Function to display the submenu for "Room-1"
void displayRoom1SubMenu() {
    display.clearDisplay();
    display.setTextSize(1); 
    display.setTextColor(WHITE);

    // Loop through submenu items and display them
    for (int i = 0; i < room1MenuItems; i++) {
        if (i == currentMenu) {
            display.setCursor(0, i * 16); // Highlight current submenu item
            display.print("> "); // Arrow indicating the selected option
        } else {
            display.setCursor(18, i * 16); // Adjust X offset for other items
        }
        display.println(room1SubMenuItems[i]);
    }
  
    display.display();
}

// Function to display the sensor submenu
void displaySensorSubMenu() {
    display.clearDisplay();
    display.setTextSize(1); 
    display.setTextColor(WHITE);

    // Loop through sensor submenu items and display them
    for (int i = 0; i < 3; i++) {
        if (i == currentMenu) {
            display.setCursor(0, i * 16); // Highlight current sensor item
            display.print("> "); // Arrow indicating the selected option
        } else {
            display.setCursor(18, i * 16); // Adjust X offset for other items
        }
        display.println(sensorSubMenuItems[i]);
    }

    display.display();
}

void displaySettings() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  for (int i = 0; i < settingSubmenuItems; i++) {
    if(i == currentMenu){
      display.setCursor(0, i * 16);
      display.print("> ");
    } else {
      display.setCursor(18, i * 16);
    }
    display.println(settingsList[i]);
  }
  display.display();
}

void setup() {
    Serial.begin(115200);
    dht.begin();

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;) ;
    }

    // Initialize buttons
    pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
    pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
    pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_BACK_PIN, INPUT_PULLUP); // Initialize Back button

    displayMainMenu(); // Show initial menu
}

void loop() {
    // Kiểm tra nếu nút UP được nhấn
    if (digitalRead(BUTTON_UP_PIN) == LOW && !buttonUpPressed) {
        Serial.println("UP button pressed"); 
        currentMenu--;
        if (currentMenu < 0) {
            currentMenu = (inMainMenu) ? totalMenuItems - 1 : (inSensorMenu ? 2 : (inSettingsMenu ? settingSubmenuItems - 1 : room1MenuItems - 1));
        }
        if (inMainMenu) {
            displayMainMenu();
        } else if (inSensorMenu) {
            displaySensorSubMenu();
        } else if (inSettingsMenu) {
            displaySettings();
        } else {
            displayRoom1SubMenu();
        }
        buttonUpPressed = true; 
    } else if (digitalRead(BUTTON_UP_PIN) == HIGH) {
        buttonUpPressed = false; 
    }

    // Kiểm tra nếu nút DOWN được nhấn
    if (digitalRead(BUTTON_DOWN_PIN) == LOW && !buttonDownPressed) {
        Serial.println("DOWN button pressed"); 
        currentMenu++;
        if (currentMenu >= (inMainMenu ? totalMenuItems : (inSensorMenu ? 3 : (inSettingsMenu ? settingSubmenuItems : room1MenuItems)))) {
            currentMenu = 0;
        }
        if (inMainMenu) {
            displayMainMenu();
        } else if (inSensorMenu) {
            displaySensorSubMenu();
        } else if (inSettingsMenu) {
            displaySettings();
        } else {
            displayRoom1SubMenu();
        }
        buttonDownPressed = true;
    } else if (digitalRead(BUTTON_DOWN_PIN) == HIGH) {
        buttonDownPressed = false; 
    }

    // Xử lý nút SELECT
    if (digitalRead(BUTTON_SELECT_PIN) == LOW && !buttonSelectPressed) {
        Serial.println("SELECT button pressed");

        if (inMainMenu) {
            if (currentMenu == 0) {
                inMainMenu = false;
                currentMenu = 0;
                displayRoom1SubMenu();
            }
        } else if (!inMainMenu && currentMenu == 0) {
            inSensorMenu = true;
            currentMenu = 0;
            displaySensorSubMenu();
        } else if (!inMainMenu && currentMenu == 2) {
            // Vào menu cài đặt
            inSettingsMenu = true;
            currentMenu = 0;
            displaySettings();
        }

        buttonSelectPressed = true;
    } else if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
        buttonSelectPressed = false; 
    }

    // Xử lý nút BACK
    if (digitalRead(BUTTON_BACK_PIN) == LOW) {
        Serial.println("BACK button pressed");

        if (inSettingsMenu) {
            inSettingsMenu = false; // Thoát khỏi menu cài đặt
            currentMenu = 0;
            displayRoom1SubMenu(); // Quay lại Room-1 submenu
        } else if (inSensorMenu) {
            inSensorMenu = false;
            currentMenu = 0;
            displayRoom1SubMenu();
        } else if (!inMainMenu) {
            inMainMenu = true;
            currentMenu = 0;
            displayMainMenu();
        }
        delay(200); // Tránh việc nhấn nhiều lần
    }
}
