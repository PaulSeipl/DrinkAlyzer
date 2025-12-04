/*
  Nano 33 BLE Sense - Internal Color Sensor Test
  Note: You MUST shine a light on the object you are scanning!
*/

#include <Arduino_APDS9960.h>

void setup() {
  Serial.begin(115200);
  
  // Safety timeout: Wait for Serial, but not forever (prevents freezing)
  long start = millis();
  while (!Serial && (millis() - start < 5000)); 

  Serial.println("Initializing Internal APDS9960...");

  if (!APDS.begin()) {
    Serial.println("Error: Internal sensor not responding.");
    Serial.println("Try unplugging/replugging the USB cable.");
    while (true); // Stop here
  }
  
  Serial.println("Sensor Ready.");
  Serial.println("R, G, B, Brightness (Alpha)");
}

void loop() {
  if (APDS.colorAvailable()) {
    int r, g, b, a;
    APDS.readColor(r, g, b, a);

    // Filter out "noise" (dark readings)
    if (a > 10) {
      Serial.print(r);
      Serial.print(", ");
      Serial.print(g);
      Serial.print(", ");
      Serial.print(b);
      Serial.print(", ");
      Serial.println(a); // Alpha = Brightness

      // Visual Feedback: Is it too dark?
      if (a < 50) {
        Serial.println("Warning: Too Dark! Shine a light.");
      }
    }
  }
  delay(1000);
}