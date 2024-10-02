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

// Track if we are in the main menu, Room-1 submenu, or sensor submenu
bool inMainMenu = true; 
bool inSensorMenu = false; // Track if we are in the sensor menu
int room1MenuItems = 3;

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
    // Check if the UP button is pressed
    if (digitalRead(BUTTON_UP_PIN) == LOW && !buttonUpPressed) {
        Serial.println("UP button pressed"); // Print message when UP is pressed
        currentMenu--;
        if (currentMenu < 0) {
            currentMenu = (inMainMenu) ? totalMenuItems - 1 : (inSensorMenu ? 2 : room1MenuItems - 1);
        }
        if (inMainMenu) {
            displayMainMenu();
        } else if (inSensorMenu) {
            displaySensorSubMenu();
        } else {
            displayRoom1SubMenu();
        }
        buttonUpPressed = true; // Mark button as pressed
    } else if (digitalRead(BUTTON_UP_PIN) == HIGH) {
        buttonUpPressed = false; // Reset button state when released
    }

    // Check if the DOWN button is pressed
    if (digitalRead(BUTTON_DOWN_PIN) == LOW && !buttonDownPressed) {
        Serial.println("DOWN button pressed"); // Print message when DOWN is pressed
        currentMenu++;
        if (currentMenu >= (inMainMenu ? totalMenuItems : (inSensorMenu ? 3 : room1MenuItems))) {
            currentMenu = 0;
        }
        if (inMainMenu) {
            displayMainMenu();
        } else if (inSensorMenu) {
            displaySensorSubMenu();
        } else {
            displayRoom1SubMenu();
        }
        buttonDownPressed = true; // Mark button as pressed
    } else if (digitalRead(BUTTON_DOWN_PIN) == HIGH) {
        buttonDownPressed = false; // Reset button state when released
    }

    // Check if the SELECT button is pressed
    if (digitalRead(BUTTON_SELECT_PIN) == LOW && !buttonSelectPressed) {
        Serial.println("SELECT button pressed"); // Print message when SELECT is pressed

        if (inMainMenu) {
            // If in the main menu, check if "Room-1" is selected
            if (currentMenu == 0) {
                inMainMenu = false; // Switch to the submenu for "Room-1"
                currentMenu = 0; // Reset menu index for the submenu
                displayRoom1SubMenu();
            } else {
                Serial.print("Selected: ");
                Serial.println(mainMenuItems[currentMenu]); // Print selected main menu item
            }
        } else if (!inMainMenu && currentMenu == 0) {
            // If "Sensor" is selected in Room-1 submenu
            inSensorMenu = true; // Switch to the sensor submenu
            currentMenu = 0; // Reset menu index for the sensor submenu
            displaySensorSubMenu();
        
        }

        buttonSelectPressed = true; // Mark button as pressed
    } else if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
        buttonSelectPressed = false; // Reset button state when released
    }

    // Check if the BACK button is pressed
    if (digitalRead(BUTTON_BACK_PIN) == LOW) {
        Serial.println("BACK button pressed");

        if (inSensorMenu) {
            inSensorMenu = false; // Exit the sensor menu
            currentMenu = 0;
            displayRoom1SubMenu(); // Go back to Room-1 submenu
        } else if (!inMainMenu) {
            inMainMenu = true; // Go back to the main menu
            currentMenu = 0;
            displayMainMenu();
        }
        delay(200); // Delay to avoid multiple presses
    }
}
