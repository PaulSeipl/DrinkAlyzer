#include <Wire.h>
#include "FastIMU.h"

// --- 1. IMU SETTINGS ---
#define IMU_ADDRESS 0x68
MPU6886 IMU;
calData calib = { 0 }; 
AccelData accelData;
GyroData gyroData;

float gyroX_offset = 0;
float gyroY_offset = 0;
float gyroZ_offset = 0;

const float GYRO_LIFT_TRIGGER = 40.0;
const float GYRO_LOWER_TRIGGER = -40.0;
const float GYRO_QUIET = 10.0;
const float ACCEL_TILT_THRESH = 0.60;

enum State { IDLE, LIFTING, AT_MOUTH, LOWERING };
State currentState = IDLE;
unsigned long sipStartTime = 0;
unsigned long sipDuration = 0;

// --- 2. BUTTON INTERRUPT SETTINGS ---
const int buttonPin = 2;

// Volatile flag to switch modes
volatile bool systemActive = false; 

// Debounce variables
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; 

// --- FUNCTION PROTOTYPES ---
void toggleSystem();
void runGyroLogic();

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // --- IMU SETUP ---
  Wire.begin();
  Wire.setClock(400000);
  IMU.init(calib, IMU_ADDRESS);

  // --- CALIBRATION ---
  Serial.println("-------------------------------------");
  Serial.println("   CALIBRATING GYRO (Keep Still!)    ");
  Serial.println("-------------------------------------");
  delay(2000); 

  float xSum = 0, ySum = 0, zSum = 0;
  int sampleCount = 500;

  for (int i = 0; i < sampleCount; i++) {
    IMU.update();
    IMU.getGyro(&gyroData);
    xSum += gyroData.gyroX;
    ySum += gyroData.gyroY;
    zSum += gyroData.gyroZ;
    delay(2);
  }

  gyroX_offset = xSum / sampleCount;
  gyroY_offset = ySum / sampleCount;
  gyroZ_offset = zSum / sampleCount;

  Serial.println("Calibration Done!");
  Serial.println("System is currently OFF. Press button to start.");

  // --- BUTTON/LED SETUP ---
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Initialize LED based on starting state
  digitalWrite(LED_BUILTIN, systemActive ? HIGH : LOW);

  // Attach Interrupt
  attachInterrupt(digitalPinToInterrupt(buttonPin), toggleSystem, RISING);
}

void loop() {
  if (systemActive) {
    runGyroLogic();
    delay(20); // Control the sampling rate
  } else {
    // Optional: Reset logic if we were in the middle of a sip when turned off
    if (currentState != IDLE) {
      currentState = IDLE;
      Serial.println("System Paused - State Reset");
    }
    delay(100); // Save power when idle
  }
}

// --- LOGIC FUNCTIONS ---

void runGyroLogic() {
  IMU.update();
  IMU.getAccel(&accelData);
  IMU.getGyro(&gyroData);

  float Gx = gyroData.gyroX - gyroX_offset;
  float Ay = accelData.accelY;

  switch (currentState) {
    case IDLE:
      if (Gx > GYRO_LIFT_TRIGGER) {
        Serial.println("--> Lifting...");
        currentState = LIFTING;
      }
      break;

    case LIFTING:
      if (abs(Gx) < GYRO_QUIET) {
        if (Ay > ACCEL_TILT_THRESH) {
          Serial.println("--> At Mouth (Sipping started)");
          sipStartTime = millis();
          currentState = AT_MOUTH;
        } else {
          Serial.println("X False Alarm");
          currentState = IDLE;
        }
      }
      break;

    case AT_MOUTH:
      if (Gx < GYRO_LOWER_TRIGGER || Ay < (ACCEL_TILT_THRESH - 0.1)) {
        sipDuration = millis() - sipStartTime;
        Serial.println("--> Lowering...");
        currentState = LOWERING;
      }
      break;

    case LOWERING:
      if (abs(Gx) < GYRO_QUIET) {
        Serial.print("COMPLETED! Sip Duration: ");
        Serial.print(sipDuration);
        Serial.println(" ms");
        currentState = IDLE; 
      }
      break;
  }
}

// --- INTERRUPT SERVICE ROUTINE ---
void toggleSystem() {
  unsigned long currentTime = millis();

  if ((currentTime - lastDebounceTime) > debounceDelay) {
    systemActive = !systemActive;
    digitalWrite(LED_BUILTIN, systemActive ? HIGH : LOW);
    lastDebounceTime = currentTime;
  }
}