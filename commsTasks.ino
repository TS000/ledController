#include <Adafruit_seesaw.h>
#include <Bounce2.h>
#include <Adafruit_SSD1351.h>
#include <Adafruit_GFX.h>
#include <ArduinoBLE.h>

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

const int ENCODER_INTERRUPT_PIN = 24;
const int SERIAL_BAUD_RATE = 115200;
Adafruit_seesaw ss;

int32_t encoder_position;
int ENCODER_MODE = false;
int bpmAdjust = 0;
int currentBPM;  // Store the current BPM
int adjustedBPM;
int persistantBPM;

// Define menu headers
static const char* menuHeaders[] = {
  "_touch:",
  "Animate",
  "Pattern",
  "Devices"
};

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
unsigned long previousTimer = 0;       // Initialize a variable to store the last time the code ran
const unsigned long timerInter = 500;  // Interval in milliseconds (1 second)

// Variables to track the current menu state and the selected menu item
MenuState currentMenu = MAIN_MENU;

int selectedMenuItem = 1;       // Start with the first menu item selected
bool menuItemSelected = false;  // Flag to track menu item selection

bool screenNeedsUpdate = true;

void setup() {

  // if (!ss.begin(0x36)) {
  //   Serial.println("Couldn't find seesaw on default address");
  //   while (1) delay(10);
  // };

  // ss.enableEncoderInterrupt(ENCODER_INTERRUPT_PIN);

  // encoder_position = ss.getEncoderPosition();

  pinMode(LED_BUILTIN, OUTPUT);    // set LED pin as output
  digitalWrite(LED_BUILTIN, LOW);  // switch off LED pin
  Serial1.begin(9600);
}

void loop() {
  unsigned long currentTimer = millis();  // Get the current time

  if (currentTimer - previousTimer >= timerInter) {
    // It's time to run your code
    previousTimer = currentTimer;  // Update the previous time

    // Your code to run at the specified interval
    // This code should not block and execute quickly
    if (Serial1.available() > 0) {
      bitOne = Serial1.read();
    }
    if (Serial1.available() > 0) {
      bitTwo = Serial1.read();
    }
    if (Serial1.available() > 0) {
      bitThree = Serial1.read();
    }

    String concatenatedString = String(bitOne) + String(bitTwo) + String(bitThree);
    currentBPM = concatenatedString.toInt();
  }

  Serial.println(currentBPM);
}