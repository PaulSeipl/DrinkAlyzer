#include <Wire.h>
#include "FastIMU.h"

#define IMU_ADDRESS 0x68

MPU6886 IMU;
calData calib = { 0 }; 
AccelData accelData;
GyroData gyroData;

// Offsets to fix the Gyro drift
float gyroX_offset = 0;
float gyroY_offset = 0;
float gyroZ_offset = 0;


// --- THRESHOLDS ---
const float GYRO_LIFT_TRIGGER = 40.0;   // Spike when lifting
const float GYRO_LOWER_TRIGGER = -40.0; // Spike when putting down (Negative!)
const float GYRO_QUIET = 10.0;          // Threshold for "stillness"
const float ACCEL_TILT_THRESH = 0.60;   // Minimum tilt to count as "at mouth"

// --- STATES ---
enum State {
  IDLE,
  LIFTING,
  AT_MOUTH,
  LOWERING
};

State currentState = IDLE;
unsigned long sipStartTime = 0;
unsigned long sipDuration = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Wire.begin();
  Wire.setClock(400000);

  IMU.init(calib, IMU_ADDRESS);

  Serial.println("-------------------------------------");
  Serial.println("   CALIBRATING GYRO (Keep Still!)    ");
  Serial.println("-------------------------------------");
  Serial.println("Place sensor flat and DO NOT MOVE it.");
  delay(2000); // Time to place it down

  Serial.println("Calibrating...");
  
  float xSum = 0, ySum = 0, zSum = 0;
  int sampleCount = 500;

  // Take 500 samples to find the average drift
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

  Serial.println("Done! Starting loop...");
}

void loop() {
  IMU.update();
  IMU.getAccel(&accelData);
  IMU.getGyro(&gyroData);

//   --- 1. ACCELEROMETER (Gravity/Tilt) ---
//   X and Y should be ~0 when flat. Z should be ~1.00 (gravity).
//   These show TILT or MOVEMENT.
//   Serial.print("A_X: "); Serial.print(accelData.accelX);
//   Serial.print("  A_Y: "); Serial.print(accelData.accelY);
//   Serial.print("  A_Z: "); Serial.print(accelData.accelZ);

  // --- 2. GYROSCOPE (Rotation Speed) ---
  // We subtract the offset so it reads 0.00 when still.
  // These only change when you TWIST/ROTATE the sensor.
  float Gx = gyroData.gyroX - gyroX_offset;
  float Gy = gyroData.gyroY - gyroY_offset;
  float Gz = gyroData.gyroZ - gyroZ_offset;
  float Ay = accelData.accelY;

  // --- STATE MACHINE ---
  
  switch (currentState) {
    
    // 1. WAITING ON TABLE
    case IDLE:
      // Wait for strong UPWARD rotation
      if (Gx > GYRO_LIFT_TRIGGER) {
        Serial.println("--> Lifting...");
        currentState = LIFTING;
      }
      break;

    // 2. MOVING UPWARDS
    case LIFTING:
      // Wait for the movement to stop (stabilize)
      if (abs(Gx) < GYRO_QUIET) {
        // We stopped moving. Are we tilted high enough?
        if (Ay > ACCEL_TILT_THRESH) {
          Serial.println("--> At Mouth (Sipping started)");
          sipStartTime = millis(); // START TIMER
          currentState = AT_MOUTH;
        } else {
          Serial.println("X False Alarm (Not tilted enough)");
          currentState = IDLE;
        }
      }
      break;

    // 3. DRINKING (MEASURING TIME)
    case AT_MOUTH:
      // We stay here as long as the cup is steady.
      // Exit Condition A: Strong DOWNWARD rotation (putting it down fast)
      // Exit Condition B: Tilt drops (putting it down slowly)
      
      if (Gx < GYRO_LOWER_TRIGGER || Ay < (ACCEL_TILT_THRESH - 0.1)) {
        sipDuration = millis() - sipStartTime; // STOP TIMER
        Serial.println("--> Lowering...");
        currentState = LOWERING;
      }
      break;

    // 4. MOVING DOWNWARDS
    case LOWERING:
      // Wait for the sensor to settle back on the table
      // We check if rotation is close to 0 again
      if (abs(Gx) < GYRO_QUIET) {
        Serial.println("-----------------------------");
        Serial.print("COMPLETED! Sip Duration: ");
        Serial.print(sipDuration);
        Serial.println(" ms");
        Serial.println("-----------------------------");
        
        // Reset for next sip
        currentState = IDLE; 
      }
      break;
  }

  delay(20); // Faster loop for better responsiveness

  // Serial.print("   |   G_X: "); Serial.print(Gx);
  // Serial.print("  G_Y: "); Serial.print(Gy);
  // Serial.print("  G_Z: "); Serial.println(Gz);

  // delay(100);
}