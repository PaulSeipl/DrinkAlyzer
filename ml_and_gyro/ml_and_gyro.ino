/* Includes ---------------------------------------------------------------- */
#include <Wire.h>
#include <ArduinoBLE.h>
#include <Arduino_APDS9960.h>
#include <vector>
#include <tuple>
#include <numeric>

// Include your custom libraries
#include "FastIMU.h"
#include <DrinkAlyzer_inferencing.h> // Ensure this library is installed/in folder

// ===================================================================================
//                                   CONFIGURATION
// ===================================================================================

// --- 1. GYRO / IMU SETTINGS ---
#define IMU_ADDRESS 0x68
MPU6886 IMU;
calData calib = { 0 }; 
AccelData accelData;
GyroData gyroData;

// Gyro Offsets & Thresholds
float gyroX_offset = 0, gyroY_offset = 0, gyroZ_offset = 0;
const float GYRO_LIFT_TRIGGER = 40.0;
const float GYRO_LOWER_TRIGGER = -40.0;
const float GYRO_QUIET = 10.0;
const float ACCEL_TILT_THRESH = 0.60;

// Gyro State Machine
enum State { IDLE, LIFTING, AT_MOUTH, LOWERING };
State currentState = IDLE;
unsigned long sipStartTime = 0;
unsigned long sipDuration = 0;

// --- 2. ML / BLE SETTINGS ---
#define SERVICE_UUID        "19B10000-E8F2-537E-4F6C-D104768A1214"
#define CHARACTERISTIC_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"

BLEService drinkService(SERVICE_UUID);
BLEStringCharacteristic drinkCharacteristic(CHARACTERISTIC_UUID, BLERead | BLENotify, 20);

const int sensorLedPin = 5; // LED for illuminating liquid
const int moisturePin = A1;

// ML Thresholds
const float TRAIN_SET_MAX = 491.4;
const float THRESHOLD_PERCENT = 0.05; 
const int MAX_READINGS = 5; 

// ML Data Buffers
std::vector<int> cap_readings;
std::vector<std::tuple<float, float, float>> rgb_readings;

// Timer for ML loop (to replace delay and keep button responsive)
unsigned long lastMLRunTime = 0;
const int ML_INTERVAL = 1000; // Run inference every 1 second

// --- 3. SYSTEM CONTROL ---
const int buttonPin = 2;
volatile bool systemModeHigh = false; // FALSE = ML Mode, TRUE = Gyro Mode
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// ===================================================================================
//                                     SETUP
// ===================================================================================

void toggleSystem(); // Forward declaration

void setup() {
  Serial.begin(115200);
  // while (!Serial); // Commented out for battery usage

  // --- HARDWARE INIT ---
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(sensorLedPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  
  digitalWrite(sensorLedPin, LOW); // Start with liquid sensor light OFF

  Wire.begin();
  Wire.setClock(400000);

  // --- 1. INIT SENSORS (IMU + APDS) ---
  IMU.init(calib, IMU_ADDRESS);
  
  if (!APDS.begin()) {
    Serial.println("Error: APDS9960 (Color Sensor) not detected.");
  }

  // --- 2. INIT BLE ---
  if (!BLE.begin()) {
    Serial.println("Error: BLE failed!");
    while(1);
  }
  
  BLE.setLocalName("Sommelier_AI");
  BLE.setAdvertisedService(drinkService);
  drinkService.addCharacteristic(drinkCharacteristic);
  BLE.addService(drinkService);
  drinkCharacteristic.writeValue("Ready");
  BLE.advertise();
  
  Serial.println("BLE Active. Waiting for connection...");

  // --- 3. CALIBRATE GYRO ---
  Serial.println(">> CALIBRATING GYRO (Keep Flat & Still) <<");
  delay(1000); 

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
  Serial.println("Current Mode: ML Analysis (Press button to switch to Sip Detection)");

  // --- 4. ATTACH INTERRUPT ---
  attachInterrupt(digitalPinToInterrupt(buttonPin), toggleSystem, RISING);
}

// ===================================================================================
//                                   MAIN LOOP
// ===================================================================================

void loop() {
  // BLE.poll(); // Keep BLE connection alive

  if (systemModeHigh) {
    // === MODE A: GYRO / SIP DETECTION ===
    // Turn off the ML sensor light to save power/confusion
    digitalWrite(sensorLedPin, LOW); 
    
    runGyroLogic();
    delay(20); // Fast sampling for motion

  } else {
    // === MODE B: ML / LIQUID ANALYSIS ===
    // Turn on the sensor light
    digitalWrite(sensorLedPin, HIGH);

    runMLLogic();
    // We do NOT use delay() here. The runMLLogic function uses a non-blocking timer.
  }
}

// ===================================================================================
//                                LOGIC FUNCTIONS
// ===================================================================================

// --- FUNCTION 1: GYRO LOGIC ---
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

// --- FUNCTION 2: ML / BLE LOGIC ---
void runMLLogic() {
  // Non-blocking timer: Only run every 1 second (ML_INTERVAL)
  // This allows the button interrupt to work instantly even in ML mode
  if (millis() - lastMLRunTime < ML_INTERVAL) {
    return;
  }
  lastMLRunTime = millis();

  // 1. Gather Sensor Data
  cap_readings.push_back(analogRead(moisturePin));
  if (cap_readings.size() > MAX_READINGS) cap_readings.erase(cap_readings.begin());

  if (APDS.colorAvailable()) {
      int r, g, b, a;
      APDS.readColor(r, g, b, a);
      float sum = r + g + b;
      if (sum > 0) {
          rgb_readings.push_back(std::make_tuple(r/sum, g/sum, b/sum));
      } else {
          rgb_readings.push_back(std::make_tuple(0.0f, 0.0f, 0.0f));
      }
      if (rgb_readings.size() > MAX_READINGS) rgb_readings.erase(rgb_readings.begin());
  }

  // 2. Check Data Sufficiency
  if (cap_readings.size() == MAX_READINGS && rgb_readings.size() == MAX_READINGS) {
      
      // Compute Averages
      float avg_cap = 0;
      for(int v : cap_readings) avg_cap += v;
      avg_cap /= MAX_READINGS;

      float avg_r = 0, avg_g = 0, avg_b = 0;
      for(auto& t : rgb_readings) {
          avg_r += std::get<0>(t);
          avg_g += std::get<1>(t);
          avg_b += std::get<2>(t);
      }
      avg_r /= MAX_READINGS;
      avg_g /= MAX_READINGS;
      avg_b /= MAX_READINGS;

      // 3. Prepare Features
      float features[] = { avg_cap, avg_r, avg_g, avg_b };
      signal_t signal;
      int err = numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
      
      if (err != 0) { Serial.println("DSP Error"); return; }

      // 4. Run Inference
      ei_impulse_result_t result = { 0 };
      EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

      if (res != EI_IMPULSE_OK) { Serial.println("Classifier Error"); return; }

      // 5. Find Best Prediction
      float max_score = 0.0;
      int max_index = -1;
      String bestLabel = "Unknown";

      for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
          if (result.classification[i].value > max_score) {
              max_score = result.classification[i].value;
              max_index = i;
          }
      }

      if (max_index >= 0) {
          bestLabel = String(result.classification[max_index].label);
          float threshold_val = TRAIN_SET_MAX + TRAIN_SET_MAX * THRESHOLD_PERCENT;

          if (avg_cap > threshold_val) bestLabel = "Unknown";

          Serial.print("Capacitive: ");
            Serial.println(avg_cap);

            Serial.print("Color: ");
            Serial.print(avg_r);
            Serial.print(" ");
            Serial.print(avg_g);
            Serial.print(" ");
            Serial.println(avg_b);

          Serial.print("Detected: "); Serial.print(bestLabel);
          Serial.print(" ("); Serial.print(max_score * 100); Serial.println("%)");

          // 6. Send to Bluetooth
          BLEDevice central = BLE.central();
          if (central && central.connected()) {
              String payload = bestLabel + "," + String(max_score, 2); 
              drinkCharacteristic.writeValue(payload);
          }
      }
  }
}

// --- INTERRUPT SERVICE ROUTINE ---
void toggleSystem() {
  unsigned long currentTime = millis();

  if ((currentTime - lastDebounceTime) > debounceDelay) {
    // Toggle the mode
    systemModeHigh = !systemModeHigh;
    
    // Toggle the Built-in LED to indicate the change
    digitalWrite(LED_BUILTIN, systemModeHigh ? HIGH : LOW);
    
    lastDebounceTime = currentTime;
  }
}