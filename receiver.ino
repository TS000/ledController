#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <Bounce2.h>
#include <WebSocketsClient_Generic.h>
#include <WiFiNINA_Generic.h>

#define DEBUG_WEBSOCKETS_PORT Serial

WebSocketsClient client;

// Sockets
const char* ssid = "wts";
const char* password = "acidflip";
const char* server = "10.42.0.1";
const int port = 1995;
const String arduinoID = "9";

WiFiClient wifiClient;

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET -1

// Screen Colors
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define DARK_RED 0x8B0000
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// You can use any (4 or) 5 pins
#define SCLK_PIN 13
#define MOSI_PIN 11
#define DC_PIN 8
#define CS_PIN 10
#define RST_PIN 9

char receivedChar;
String receivedData;

// Create an instance of the Adafruit_SSD1351 display object
Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);

int bpmAdjust = 0;
int adjustedBPM;
int persistantBPM;
int previousBPM = 1;
int currentBPM;  // Store the current BPM
int currentButton = 9;
int currentBrightness = 90;
int currentColor = 3;
int previousButton;
int previousBrightness;
int previousColor;

// Screen vars
unsigned long lastBPMUpdateTime = 0; // Global variable to store the last BPM update time
bool isDisplayVisible = true;        // Global flag to track display state

// Define menu headers
static const char* menuHeaders[] = {
    "_weird:",
    "test",
    "test",
    "test"};

// Define menu states
enum MenuState {
    MAIN_MENU,
    ANIMATE,
    NUMBERS,
    DEVICES
};

// UART
int bitOne = 1;
int bitTwo = 1;
int bitThree = 0;
int bitFour = 0;
unsigned long previousTimer = 0;       // Initialize a variable to store the last time the code ran
const unsigned long timerInter = 500;  // Interval in milliseconds (1 second)

// Variables to track the current menu state and the selected menu item
MenuState currentMenu = MAIN_MENU;

int selectedMenuItem = 1;       // Start with the first menu item selected
bool menuItemSelected = false;  // Flag to track menu item selection

bool screenNeedsUpdate = true;
unsigned long lastWebSocketConnectAttemptTime = 0;
unsigned long wifiConnectAttemptTime = 0;
unsigned long lastSendTime = 0;
String message;

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);

    tft.begin();  // Initialize the OLED display using the tft object
    tft.setRotation(3);
    tft.fillScreen(BLACK);  // Fill the screen with black
    WiFi.begin(ssid, password);

    // If the connection is successful, start the WebSocket connection
    // Connect to websocket server
    client.begin(server, port, "/");
    // String dataToSend = String(arduinoID) + "," + String(currentBPM) + "," + String(currentButton) + "," + String(currentBrightness) + "," + String(currentColor);
    // client.sendTXT(dataToSend);

    // Set up event handler
    client.begin(server, port, "/");
    client.onEvent(webSocketEvent);
}

bool isConnected = false;

void loop() {
    client.loop();
    connectToWiFi();
    sendDataAtBPMInterval();

    receiver();

    // Consolidate BPM update checks
    if (currentBPM != previousBPM) {
        lastBPMUpdateTime = millis();
        previousBPM = currentBPM;
        isDisplayVisible = true; // Ensure display is visible when BPM is updated
        screenNeedsUpdate = true;
    }

    // Check if 3 seconds have passed since the last BPM update
    if (millis() - lastBPMUpdateTime > 10000 && isDisplayVisible) {
        tft.fillScreen(BLACK); // Turn off the display by filling it with black
        isDisplayVisible = false;
        Serial.println("Display turned off");
    }

    // Update the screen only if it is visible and needs updating
    if (isDisplayVisible && screenNeedsUpdate) {
        displayMenu(currentBPM, currentButton);
        screenNeedsUpdate = false;
        Serial.println("Display updated");
    }

      // Check if a menu item has been selected
    if (menuItemSelected) {
        switch (selectedMenuItem) {
            case 1:
                currentMenu = ANIMATE;
                break;
            case 2:
                currentMenu = NUMBERS;
                break;
            case 3:
                currentMenu = DEVICES;
                break;
        }

        // Reset the flag
        menuItemSelected = false;
    }
}

//======= Menu =======
void displayMenuHeader() {
    tft.setTextColor(WHITE, BLACK);  // Set text color

    // Display the selected menu header
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println(menuHeaders[currentMenu]);
}

void displayMenuItem(int itemIndex, bool selected) {
    tft.setTextSize(1);
    tft.setTextColor(selected ? RED : WHITE);  // Highlight the selected item
    tft.setCursor(10, 40 + (itemIndex * 20));
    tft.println(menuHeaders[itemIndex]);
}

void displayMenu(int bpm, int currentButton) {
    displayMenuHeader();

    bigBPM(bpm, currentButton);

    for (int i = 1; i <= 3; i++) {
        displayMenuItem(i, i == selectedMenuItem);
    }
}

void bigBPM(int bpm, int button) {
    tft.setTextColor(RED, BLACK);
    tft.setCursor(10, 10);
    // tft.print("BPM:");
    tft.setTextSize(3);
    tft.setCursor(70, 57);
    tft.print(bpm);
    tft.setTextSize(2);
    tft.setTextColor(WHITE, BLACK);
    tft.setCursor(70, 93);
    tft.print(button);
    tft.print("  ");
}

enum ReceiverState {
    WAIT_FOR_HEADER,
    WAIT_FOR_BPM_DATA,
    WAIT_FOR_TWO_DIGIT_BPM_DATA,
    WAIT_FOR_BUTTON_DATA,
    WAIT_FOR_BRIGHTNESS_DATA,
    WAIT_FOR_COLOR_DATA
};

ReceiverState receiverState = WAIT_FOR_HEADER;
byte headerByte;

void receiver() {
    switch (receiverState) {
        case WAIT_FOR_HEADER:
            if (Serial1.available() > 0) {
                headerByte = Serial1.read();
                if (headerByte == 0x01) {
                    receiverState = WAIT_FOR_BPM_DATA;
                } else if (headerByte == 0x02) {
                    receiverState = WAIT_FOR_BUTTON_DATA;
                } else if (headerByte == 0x03) {
                    receiverState = WAIT_FOR_TWO_DIGIT_BPM_DATA;
                } else if (headerByte == 0x04) {  // New condition for Brightness data
                    receiverState = WAIT_FOR_BRIGHTNESS_DATA;
                } else if (headerByte == 0x05) {  // New condition for Color data
                    receiverState = WAIT_FOR_COLOR_DATA;
                } else {
                    Serial.println("Received unexpected headerByte");
                }
            }
            break;

         case WAIT_FOR_BPM_DATA:
            if (Serial1.available() >= 2) {
                byte highByte = Serial1.read();
                byte lowByte = Serial1.read();

                Serial.print("Raw BPM Bytes - High: ");
                Serial.print(highByte, HEX); // Print in hexadecimal
                Serial.print(", Low: ");
                Serial.println(lowByte, HEX);

                currentBPM = (highByte << 8) | lowByte;
                Serial.print("Interpreted BPM: ");
                Serial.println(currentBPM);

                receiverState = WAIT_FOR_HEADER;
            }
            break;

        case WAIT_FOR_TWO_DIGIT_BPM_DATA:
            if (Serial1.available() >= 1) {
                byte lowByte = Serial1.read();
                currentBPM = lowByte;                                  // If high byte is 0, BPM is a two-digit number
                String bpmWithLeadingZero = "0" + String(currentBPM);  // BPM with leading zero
                screenNeedsUpdate = true;
                Serial.print("Received BPM: ");
                Serial.println(bpmWithLeadingZero);
                receiverState = WAIT_FOR_HEADER;
            }
            break;

       case WAIT_FOR_BUTTON_DATA:
            if (Serial1.available() >= 1) {
                currentButton = Serial1.read();
                
                if (!isDisplayVisible) {
                    isDisplayVisible = true;  // Turn on the display
                    screenNeedsUpdate = true; // Mark that the screen needs to be updated
                }

                Serial.print("Received Button State: ");
                Serial.println(currentButton);
                receiverState = WAIT_FOR_HEADER;
            }
            break;

        case WAIT_FOR_BRIGHTNESS_DATA:
            if (Serial1.available() >= 1) {
                currentBrightness = Serial1.read();
                Serial.print("Received Brightness: ");
                Serial.println(currentBrightness);
                receiverState = WAIT_FOR_HEADER;
            }
            break;

            // You can use any (4 or) 5 pins
        case WAIT_FOR_COLOR_DATA:
            if (Serial1.available() >= 1) {
                currentColor = Serial1.read();
                Serial.print("Received Color: ");
                Serial.println(currentColor);
                receiverState = WAIT_FOR_HEADER;
            }
            break;
    }
}

// Websockets
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("[WebSocket] Disconnected");
            isConnected = false;
            break;
        case WStype_CONNECTED:
            Serial.print("[WebSocket] Connected to: ");
            Serial.println((char*)payload);
            isConnected = true;
            break;
        case WStype_TEXT:
            Serial.print("[WebSocket] Text: ");
            for (size_t i = 0; i < length; i++) {
                Serial.print((char)payload[i]);
            }
            Serial.println();
            break;
    }
}

void connectToWiFi() {
    // Check if WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(WiFi.RSSI());
        // Attempt to connect to WiFi
        if (millis() - wifiConnectAttemptTime > 10000) {  // 10 seconds have passed since the last connection attempt
            WiFi.begin(ssid, password);
            wifiConnectAttemptTime = millis();
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected to WiFi");
        } else {
            Serial.println("Failed to connect to WiFi");
            return;  // Skip the rest of the loop
        }
    }
}

void sendDataAtBPMInterval() {
    unsigned long sendInterval = 60000 / currentBPM; // Convert BPM to interval in milliseconds

    // Check if the send interval has passed since the last send
    if (millis() - lastSendTime >= sendInterval) {
        Serial.println("Sending data to WebSocket server...");
        String dataToSend = String(arduinoID) + "," + String(currentBPM) + "," + String(currentButton) + "," + String(currentBrightness) + "," + String(currentColor);
        client.sendTXT(dataToSend);
        Serial.println("Data sent: " + dataToSend);

        // Update the time of the last send
        lastSendTime = millis();
    }
}
