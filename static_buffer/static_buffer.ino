/* Includes ---------------------------------------------------------------- */
#include <DrinkAlyzer_inferencing.h>
#include <Arduino_APDS9960.h>
#include <ArduinoBLE.h>
#include <vector>
#include <tuple>
#include <numeric>

// --- BLE CONFIGURATION ---
// These UUIDs act like a specific "Phone Number" for your app to find
#define SERVICE_UUID        "19B10000-E8F2-537E-4F6C-D104768A1214"
#define CHARACTERISTIC_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"

BLEService drinkService(SERVICE_UUID);
BLEStringCharacteristic drinkCharacteristic(CHARACTERISTIC_UUID, BLERead | BLENotify, 20);

// Hardware Pins
const int ledPin = 2;
const int moisturePin = A0;

const float TRAIN_SET_MAX = 491.4;
const float THRESHOLD_PERCENT = 0.10;      // 10%

// Data Buffers for Smoothing
const int MAX_READINGS = 5; 
std::vector<int> cap_readings;
std::vector<std::tuple<float, float, float>> rgb_readings;

/* Setup ------------------------------------------------------------------- */
void setup() {
    Serial.begin(115200);
    // while (!Serial); // Keep commented out so it runs on battery!

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH); // Turn on Light for sensing

    // 1. Init Sensors
    if (!APDS.begin()) {
        Serial.println("Error: APDS9960 not detected.");
        while (1);
    }

    // 2. Init Bluetooth
    if (!BLE.begin()) {
        Serial.println("starting BLE failed!");
        while (1);
    }

    // 3. Configure BLE Service
    BLE.setLocalName("Sommelier_AI");
    BLE.setAdvertisedService(drinkService);
    drinkService.addCharacteristic(drinkCharacteristic);
    BLE.addService(drinkService);
    
    // Default value
    drinkCharacteristic.writeValue("Scanning...");
    
    BLE.advertise();
    Serial.println("Bluetooth Active. Connect your phone!");
}

/* Main Loop --------------------------------------------------------------- */
void loop() {
    // BLE.poll(); // Keep the connection alive

    // --- 1. Gather Sensor Data ---
    
    // Moisture
    cap_readings.push_back(analogRead(moisturePin));
    if (cap_readings.size() > MAX_READINGS) cap_readings.erase(cap_readings.begin());

    // Color
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

    // --- 2. Check if we have enough data to classify ---
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

        // --- 3. Prepare Features for AI ---
        float features[] = { avg_cap, avg_r, avg_g, avg_b };

        // Wrapper for Edge Impulse
        signal_t signal;
        int err = numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
        if (err != 0) {
            Serial.println("Error creating signal from buffer");
            return;
        }

        // --- 4. Run Inference ---
        ei_impulse_result_t result = { 0 };
        EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

        if (res != EI_IMPULSE_OK) {
            Serial.println("Err: Classifier failed");
            return;
        }

        // --- 5. Find Best Prediction ---
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

            float threshold_val = TRAIN_SET_MAX * THRESHOLD_PERCENT;

            if (avg_cap > threshold_val) {
                bestLabel = "Unknown";
            }

            
            // Print to Serial (for debugging)
            Serial.print("Detected: ");
            Serial.print(bestLabel);
            Serial.print(" (");
            Serial.print(max_score * 100);
            Serial.println("%)");

            // --- 6. SEND TO BLUETOOTH ---
            // Only update if a device is actually connected to save battery
            BLEDevice central = BLE.central();
            if (central && central.connected()) {
                // Create a package: "Label,Score"
                // String(max_score, 2) converts the float 0.987 to string "0.99"
                String payload = bestLabel + "," + String(max_score, 2); 
                
                drinkCharacteristic.writeValue(payload);
            }
        }
        
        delay(200); // Small delay to prevent spamming
    }
}