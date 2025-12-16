// Define the digital pin the button is connected to
const int buttonPin = 2;

// Variable to store the current state of the LED (HIGH or LOW)
int ledState = LOW;

// Variable to store the previous state of the button
int lastButtonState = HIGH; // Start HIGH because of the internal pull-up

void setup() {
  // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize the button pin as an input with the internal pull-up resistor enabled
  // This means the pin will be HIGH when the button is NOT pressed.
  pinMode(buttonPin, INPUT_PULLUP);

  // (Optional) Initialize serial communication for debugging
  // Serial.begin(9600);
}

void loop() {
  // Read the current state of the button
  int currentButtonState = digitalRead(buttonPin);

  // *** Button Toggle Logic ***

  // Check if the button state has changed from pressed (LOW) to released (HIGH)
  // This is a common way to detect a single "click" (rising edge)
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    // Button was just released, so toggle the LED state
    if (ledState == LOW) {
      ledState = HIGH;
      // Serial.println("LED ON");
    } else {
      ledState = LOW;
      // Serial.println("LED OFF");
    }

    // Update the actual LED
    digitalWrite(LED_BUILTIN, ledState);
  }

  // Save the current button state for the next loop iteration
  lastButtonState = currentButtonState;

  // Small delay to debounce the button (optional, but good practice)
  delay(10);
}