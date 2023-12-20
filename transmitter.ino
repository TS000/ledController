#include <ArduinoTapTempo.h>
#include <Bounce2.h>

// TODO:

const int BUTTON_PIN = 9;
// const int RESET_BUTTON_PIN = 8;
const int LED_PIN = A5;  // Pin for the LED
const int SERIAL_BAUD_RATE = 9600;
ArduinoTapTempo tapTempo;

const int ledPins[] = {13, A1, A2, A3, A4};  // Define an array of button pins
const int numLEDs = 5;                       // Number of LEDs
const int buttonPins[] = {4, 2, 3, 7, 8};    // Define an array of button pins
const int numButtons = 5;
int buttonChar[numButtons];

int bpmAdjust = 0;
int currentBPM = 120;  // Store the current BPM
int adjustedBPM = 120;
int persistantBPM = 120;
bool resetDownbeat = false;
unsigned long resetTime = 0;
int currentButton = 9;

unsigned long lastUpdateMillis = 0;
const unsigned long updateInterval = 10;

unsigned long ledBlinkDuration[5] = {0};

unsigned long previousMillis = 0;
unsigned long interval;

Bounce resetButton = Bounce();
Bounce buttons[5];  // Create an array of debounced buttons

bool ledState[5] = {false};       // Store the state of each LED
bool resetButtonPressed = false;  // Flag to track reset button press

unsigned long previousTimer = 0;       // Initialize a variable to store the last time the code ran
const unsigned long timerInter = 500;  // Interval in milliseconds (1 second)

// Global variables to track the last sent values
int lastSentBPM = 0;          // Initialize with an impossible BPM value
int lastSentButtonState = 9;  // Initialize with an impossible button state

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    Serial.begin(9600);  // initialize serial communication at 9600 bits per second:
    Serial1.begin(9600);

    tapTempo.setTotalTapValues(4);
    tapTempo.setBeatsUntilChainReset(8);
    tapTempo.setMaxBPM(200);
    tapTempo.setMinBPM(50);

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

        if (buttons[i].fell()) {                       // Button pressed
                                                       //   Serial.println(i);
            ledState[i] = !ledState[i];                // Toggle LED state
            digitalWrite(ledPins[i], HIGH);            // Turn on the LED
            ledBlinkDuration[i] = currentTimer + 150;  // Set the time to turn off the LED (adjustable duration)
            currentButton = i;
        }

        // Check if it's time to turn off the LED
        if (currentTimer >= ledBlinkDuration[i] && digitalRead(ledPins[i]) == HIGH) {
            digitalWrite(ledPins[i], LOW);  // Turn off the LED
        }
    }
    bool buttonDown = digitalRead(BUTTON_PIN) == LOW;
    tapTempo.update(buttonDown);

    transmitter(currentBPM, currentButton);

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
    }
}

void transmitter(int bpm, int buttonState) {
    static int lastBPM = -1;
    static int lastButtonIndex = -1;

    if (!tapTempo.isChainActive()) {
        // Send BPM if it has changed
        if (bpm != lastBPM && bpm >= 30 && bpm <= 250) {
            uint8_t header = bpm < 100 ? 0x03 : 0x01;  // Use a different header for two-digit BPM
            uint8_t bpmData[] = {
                header,                                   // Header for BPM
                static_cast<uint8_t>((bpm >> 8) & 0xFF),  // High byte of BPM
                static_cast<uint8_t>(bpm & 0xFF)          // Low byte of BPM
            };
            Serial1.write(bpmData, sizeof(bpmData));
            lastBPM = bpm;

            // Print bpmData and sizeof(bpmData)
            Serial.print("bpmData: ");
            for (int i = 0; i < sizeof(bpmData); i++) {
                Serial.print(bpmData[i], HEX);
                Serial.print(" ");
            }
            Serial.println();

            Serial.print("sizeof(bpmData): ");
            Serial.println(sizeof(bpmData));
        }
    }

    // Send Button State if it has changed
    if (currentButton != lastButtonIndex && currentButton >= 0 && currentButton <= 5) {
        uint8_t buttonData[] = {
            0x02,                                // Header for Button State
            static_cast<uint8_t>(currentButton)  // Button state
        };
        Serial1.write(buttonData, sizeof(buttonData));
        lastButtonIndex = currentButton;

        Serial.print("Button state sent: ");
        Serial.println(currentButton);
    }
}
