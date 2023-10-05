#include <ArduinoTapTempo.h>
#include <Adafruit_seesaw.h>
#include <Bounce2.h>
#include <Adafruit_SSD1351.h>
#include <Adafruit_GFX.h>
#include <ArduinoBLE.h>

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET -1

// You can use any (4 or) 5 pins
#define SCLK_PIN 13
#define MOSI_PIN 11
#define DC_PIN 8
#define CS_PIN 10
#define RST_PIN 9

// Create an instance of the Adafruit_SSD1351 display object
Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define DARK_RED 0x8B0000
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

const int BUTTON_PIN = 2;
const int ENCODER_INTERRUPT_PIN = 24;
const int RESET_BUTTON_PIN = 5;
const int LED_PIN = 3;  // Pin for the LED
const int SERIAL_BAUD_RATE = 9600;
ArduinoTapTempo tapTempo;
Adafruit_seesaw ss;

int32_t encoder_position;
int ENCODER_MODE = false;
int bpmAdjust = 0;
int currentBPM = 120;  // Store the current BPM
bool resetDownbeat = false;
unsigned long resetTime = 0;

unsigned long lastUpdateMillis = 0;
const unsigned long updateInterval = 10;

Bounce resetButton = Bounce();

bool resetButtonPressed = false;  // Flag to track reset button press

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

// Variables to track the current menu state and the selected menu item
MenuState currentMenu = MAIN_MENU;
int selectedMenuItem = 1;       // Start with the first menu item selected
bool menuItemSelected = false;  // Flag to track menu item selection

bool screenNeedsUpdate = true;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!ss.begin(0x36)) {
    Serial.println("Couldn't find seesaw on default address");
    while (1) delay(10);
  }

  tapTempo.setTotalTapValues(4);
  tapTempo.setBeatsUntilChainReset(20);
  tapTempo.setMaxBPM(200);
  tapTempo.setMinBPM(40);

  BLE.begin();  // initialize the Bluetooth® Low Energy hardware

  Serial.println("Bluetooth® Low Energy Central - LED control");

  BLE.scanForName("Invisible Touch");  // start pre scanning for peripherals

  ss.enableEncoderInterrupt(ENCODER_INTERRUPT_PIN);

  encoder_position = ss.getEncoderPosition();

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  resetButton.attach(RESET_BUTTON_PIN);
  resetButton.interval(50);  // Debounce interval for the reset button

  pinMode(LED_PIN, OUTPUT);  // Set LED pin as output

  // Initialize the OLED display
  tft.begin();  // Initialize the OLED display using the tft object
  tft.setRotation(1);
  tft.fillScreen(BLACK);  // Fill the screen with black

  delay(1000);
  tft.fillScreen(BLACK);  // Clear the display
}

void loop() {
  handleMenuNavigation();

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

  bool buttonDown = digitalRead(BUTTON_PIN) == LOW;
  tapTempo.update(buttonDown);

  resetDownbeatOnButtonPress();

  if (resetDownbeat) {
    // Restore the BPM to the stored currentBPM value
    // bpmAdjust = currentBPM - tapTempo.getBPM();
    resetDownbeat = false;
    ENCODER_MODE = true;
  }

  currentBPM = tapTempo.getBPM();

  ENCODER_MODE = false;
  int adjustedBPM = currentBPM + bpmAdjust;
 Serial.println(adjustedBPM);
  if (adjustedBPM < 20) {
    adjustedBPM = 1;
  } else if (adjustedBPM > 300) {
    adjustedBPM = 300;
  }

  displayMenu(adjustedBPM);
  controllerBpmLed(adjustedBPM);

  // updateDisplay();
}

//========================================================================================================= Menu ================= Menu
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

void displayMenu(int bpm) {
  // tft.fillScreen(BLACK);  // Add this line to update the screen
  displayMenuHeader();

  bigBPM(bpm);

  for (int i = 1; i <= 3; i++) {
    displayMenuItem(i, i == selectedMenuItem);
  }
}

void handleMenuNavigation() {
  int encoderValue = ss.getEncoderPosition();
  static int lastEncoderValue = encoderValue;

  if (resetButtonPressed) {
     adjustBPM();
  }


  if (encoderValue != lastEncoderValue && !resetButtonPressed) {
    // Rotary knob rotated, update the selected menu item
    selectedMenuItem += (encoderValue - lastEncoderValue);

    // Ensure the selectedMenuItem stays within bounds
    if (selectedMenuItem < 1) {
      selectedMenuItem = 1;
    } else if (selectedMenuItem > 3) {
      selectedMenuItem = 3;
    }

    lastEncoderValue = encoderValue;
  }

  // Handle menu item confirmation with the rotary push button
  if (!ss.digitalRead(ENCODER_INTERRUPT_PIN)) {
    menuItemSelected = true;
  }
}

void animateMenuItemSelected() {
  connectToLed();
  // Code to perform when "Animate" menu item is selected
  // Add your animation logic here
}

void numbersMenuItemSelected() {
  // Code to perform when "Numbers" menu item is selected
  // Add your numbers-related logic here
}

void devicesMenuItemSelected() {
  // Code to perform when "Devices" menu item is selected
  // Add your device-related logic here
}

void bigBPM(int bpm) {
  tft.setTextColor(RED, BLACK);
  tft.setCursor(10, 10);
  // tft.print("BPM:");
  tft.setTextSize(3);
  tft.setCursor(70, 75);
  tft.print(bpm);
}

// ========================================================================== Physical Hardware Items ============= Physical Hardware Items
void controllerBpmLed(int adjustedBPM){
  // Serial.println(adjustedBPM);
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateMillis >= updateInterval) {
    // Serial.print("bpm: ");
    // Serial.println(currentBPM);

    // Calculate the LED blink interval based on the adjusted BPM
    int ledBlinkInterval = 60000 / adjustedBPM;

    // Blink the LED
    static unsigned long previousMillis = 0;

    if (currentMillis - previousMillis >= ledBlinkInterval / 2 && !resetButtonPressed) {
      digitalWrite(LED_PIN, HIGH);
    }

    if (currentMillis - previousMillis >= ledBlinkInterval && !resetButtonPressed) {
      digitalWrite(LED_PIN, LOW);
      previousMillis = currentMillis;
    }

    lastUpdateMillis = currentMillis;
  }
}

// ==================================================================================== BPM Manipulation ================= BPM Manipulation

void adjustBPM() {
  int32_t new_position = ss.getEncoderPosition();
  if (encoder_position != new_position) {
    // Calculate the encoder movement direction
    int direction = new_position > encoder_position ? -1 : 1;  // Inverted direction for clockwise

    // "Nudge" the BPM higher or lower
    int nudgeAmount = 1;  // Adjust this value to control the nudge amount

    // Update the BPM adjustment based on direction and nudge amount
    bpmAdjust += direction * nudgeAmount;

    // Limit the BPM adjustment to a reasonable range
    if (bpmAdjust < -20) {
      bpmAdjust = -1;
    } else if (bpmAdjust > 20) {
      bpmAdjust = 1;
    }

    encoder_position = new_position;
  }
}

void resetDownbeatOnButtonPress() {
  resetButton.update();

  if (resetButton.rose()) {     // Actuate on button release
    resetButtonPressed = true;  // Set the flag to indicate the button press
  }

  if (resetButton.fell()) {
    digitalWrite(LED_PIN, LOW);  // Turn off the LED
  }

  if (resetButton.fell() && resetButtonPressed) {  // Button released after a press
    // Check if the resetDownbeat flag was not set recently (prevent repeated reset)
    if (!resetDownbeat && millis() - resetTime > 1000) {
      currentBPM = tapTempo.getBPM();
      resetDownbeat = true;
      resetTime = millis();
      resetTapTempo();  // Reset the tap tempo
    }

    resetButtonPressed = false;  // Reset the button press flag
  }
}

void resetTapTempo() {
  // Reset the tap tempo by simulating a new tap
  tapTempo.update(true);   // Simulate a button press (a tap)
  tapTempo.update(false);  // Release the button
  Serial.println("Downbeat reset.");
}

// ========================================================================================= Bluetooth ========================= Bluetooth
void connectToLed() {
  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    // discovered a peripheral, print out address, local name, and advertised service
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();

    if (peripheral.localName() != "Invisible Touch") {
      return;
    }
   controlLed(peripheral);
    // stop scanning
    BLE.stopScan();

    // peripheral disconnected, start scanning again
    // BLE.scanForUuid("3497EEC9-1282-21EB-4641-A65A3F740D8E");
    BLE.scanForName("Invisible Touch");
  }
}

void controlLed(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting...");
  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
    Serial.println(peripheral.discoverAttributes());
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // retrieve the LED characteristic
  BLECharacteristic ledCharacteristic = peripheral.characteristic("19b10000-e8f2-537e-4f6c-d104768a1214");

  if (!ledCharacteristic) {
    Serial.println("Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  } else if (!ledCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable LED characteristic!");
    peripheral.disconnect();
    return;
  }

  while (peripheral.connected()) {
    // while the peripheral is connected

    // button is released, write 0x00 to turn the LED off
    ledCharacteristic.writeValue((byte)0x00);
  }

  Serial.println("Peripheral disconnected");
}
