#include <ArduinoTapTempo.h>
#include <Bounce2.h>

// TODO:

const int BUTTON_PIN = 9;
// const int RESET_BUTTON_PIN = 8;
const int LED_PIN = A5;  // Pin for the LED
const int SERIAL_BAUD_RATE = 9600;
ArduinoTapTempo tapTempo;

const int ledPins[] = { 13, A1, A2, A3, A4 };  // Define an array of button pins
const int numLEDs = 5;                         // Number of LEDs
const int buttonPins[] = { 4, 2, 3, 7, 8 };    // Define an array of button pins
const int numButtons = 5;
int buttonChar[numButtons];

int bpmAdjust = 0;
int currentBPM = 120;  // Store the current BPM
int adjustedBPM = 120;
int persistantBPM = 120;
bool resetDownbeat = false;
unsigned long resetTime = 0;

unsigned long lastUpdateMillis = 0;
const unsigned long updateInterval = 10;

unsigned long ledBlinkDuration[5] = {0};

unsigned long previousMillis = 0;
unsigned long interval;

Bounce resetButton = Bounce();
Bounce buttons[5];  // Create an array of debounced buttons

bool ledState[5] = { false };     // Store the state of each LED
bool resetButtonPressed = false;  // Flag to track reset button press

unsigned long previousTimer = 0;       // Initialize a variable to store the last time the code ran
const unsigned long timerInter = 500;  // Interval in milliseconds (1 second)

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Serial.begin(9600);               // initialize serial communication at 9600 bits per second:
  Serial1.begin(9600);

  tapTempo.setTotalTapValues(4);
  tapTempo.setBeatsUntilChainReset(8);
  tapTempo.setMaxBPM(200);
  tapTempo.setMinBPM(40);

  // pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  // resetButton.attach(RESET_BUTTON_PIN);
  // resetButton.interval(50);  // Debounce interval for the reset button)

  for (int i = 0; i < 5; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);  // Set button pins as INPUT_PULLUP
    pinMode(ledPins[i], OUTPUT);           // Set LED pins as OUTPUT
    buttons[i].attach(buttonPins[i]);
    buttons[i].interval(50);  // Debounce interval in milliseconds
  }

  delay(1000);
}



void loop() {
  unsigned long currentTimer = millis();  // Get the current time
  for (int i = 0; i < 5; i++) {
    buttons[i].update();  // Update the button states

    if (buttons[i].fell()) {                  // Button pressed
      Serial.println(i);
      ledState[i] = !ledState[i];             // Toggle LED state
      digitalWrite(ledPins[i], HIGH);        // Turn on the LED
      ledBlinkDuration[i] = currentTimer + 200;  // Set the time to turn off the LED (adjustable duration)
      transmitter(currentBPM, i);
    }

    // Check if it's time to turn off the LED
    if (currentTimer >= ledBlinkDuration[i] && digitalRead(ledPins[i]) == HIGH) {
      digitalWrite(ledPins[i], LOW);  // Turn off the LED
    }
  }
  bool buttonDown = digitalRead(BUTTON_PIN) == LOW;
  tapTempo.update(buttonDown);

  // resetDownbeatOnButtonPress();

  // if (resetDownbeat) {
  //   // Restore the BPM to the stored currentBPM value
  //   bpmAdjust = currentBPM - tapTempo.getBPM();
  //   resetDownbeat = false;
  // }

  currentBPM = tapTempo.getBPM();
  adjustedBPM = currentBPM + bpmAdjust;
  unsigned long currentMillis = millis();

  interval = 60000 / adjustedBPM;
  // Serial.println(adjustedBPM);
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // Toggle the LED state
    // Serial.println(adjustedBPM);
  }

  if (currentTimer - previousTimer >= timerInter) {
    // It's time to run your code
    previousTimer = currentTimer;  // Update the previous time
    Serial.println(currentBPM);

    // if (currentBPM >= 50 && currentBPM <= 170) {
    //   // send bytes
    //   transmitter(currentBPM);
    // }
  }
}

void transmitter(int bpm, int buttonState) {
  if (bpm >= 100 && bpm <= 999) {
    // Send BPM value
    Serial1.write((byte)(bpm >> 8));  // High byte
    Serial1.write((byte)bpm);          // Low byte

    // Send button state
    Serial1.write((byte)buttonState);
  }



  // int encapsulatedValue = (bpm << 8) | buttonState;

  // if (bpm >= 100 && bpm <= 999 && buttonState >= 0 && buttonState <= 255) {
  //   Serial1.write(encapsulatedValue >> 8);  // Send the high byte of encapsulatedValue
  //   Serial1.write(encapsulatedValue & 0xFF);  // Send the low byte of encapsulatedValue
  // } else {
  //   // Send a placeholder value if data is not ready
  //   Serial1.write(0xFF);  // High byte placeholder
  //   Serial1.write(0xFF);  // Low byte placeholder
  // }
}

// ========================================================================== Physical Hardware Items ============= Physical Hardware Items
// void controllerBpmLed() {
//   static bool ledState = false;
//   ledState = ledState;
//   digitalWrite(LED_PIN, ledState);
// }

// ==================================================================================== BPM Manipulation ================= BPM Manipulation

// void resetDownbeatOnButtonPress() {
//   // Serial.println('reset BPM');
//   resetButton.update();

//   if (resetButton.rose()) {     // Actuate on button release
//     resetButtonPressed = true;  // Set the flag to indicate the button press
//   }

//   if (resetButton.fell()) {
//     digitalWrite(LED_PIN, LOW);  // Turn off the LED
//   }

//   if (resetButton.fell() && resetButtonPressed) {  // Button released after a press
//     // Check if the resetDownbeat flag was not set recently (prevent repeated reset)
//     if (!resetDownbeat && millis() - resetTime > 1000) {
//       currentBPM = tapTempo.getBPM();
//       resetDownbeat = true;
//       resetTime = millis();
//       resetTapTempo();  // Reset the tap tempo
//     }

//     resetButtonPressed = false;  // Reset the button press flag
//   }
// }

// void resetTapTempo() {
//   // Reset the tap tempo by simulating a new tap
//   tapTempo.update(true);   // Simulate a button press (a tap)
//   tapTempo.update(false);  // Release the button
//   // Serial.println("Downbeat reset.");
// }
