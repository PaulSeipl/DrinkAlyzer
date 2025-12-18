#include <DS18B20.h>
#include <vector>
#include <tuple>
#include <numeric>
#include <Arduino_APDS9960.h>

const int START_BUTTON_PIN = 5; 

// Vectors to store the last 5 readings
std::vector<int> capacitive_readings;

// CHANGE 1: Change tuple types from <int, int, int> to <float, float, float>
std::vector<std::tuple<float, float, float>> rgb_readings;

const int MAX_READINGS = 5; 
bool logging_active = true; 
bool last_button_state = HIGH; 

// --- Averaging Functions ---
float calculate_average(const std::vector<int>& data) {
    if (data.empty()) return 0.0;
    long sum = std::accumulate(data.begin(), data.end(), 0L); 
    return (float)sum / data.size();
}

// CHANGE 2: Update averaging function to handle floats and return floats
std::tuple<float, float, float> calculate_tuple_average(const std::vector<std::tuple<float, float, float>>& data) {
    if (data.empty()) return std::make_tuple(0.0f, 0.0f, 0.0f);

    float sum_r = 0.0f;
    float sum_g = 0.0f;
    float sum_b = 0.0f;

    for (const auto& item : data) {
        sum_r += std::get<0>(item);
        sum_g += std::get<1>(item);
        sum_b += std::get<2>(item);
    }

    // Debug print
    // Serial.print("sum_r: "); Serial.println(sum_r);

    size_t count = data.size();
    
    return std::make_tuple(
        sum_r / count, 
        sum_g / count, 
        sum_b / count
    );
}

void setup() {
  Serial.begin(9600); 
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  
    if (!APDS.begin()) {
        Serial.print(F("Error initializing APDS9960 sensor."));
        while (!APDS.begin()) {
        Serial.print(F("."));
        delay(500);
        }
    }
  Serial.println("Ready. Press button on D1 to start logging...");
}

void loop() {
    // 1. Check Button State
    int current_button_state = digitalRead(START_BUTTON_PIN);
    
    if (current_button_state == LOW && last_button_state == HIGH) {
        logging_active = true; // Toggle properly
        
        if (logging_active) {
            Serial.println();
            Serial.println("--- LOGGING STARTED ---");
            // Added headers for RGB
            Serial.println("Capacitive,Red_Ratio,Green_Ratio,Blue_Ratio");
        } else {
            Serial.println("--- LOGGING STOPPED ---");
            capacitive_readings.clear();
            rgb_readings.clear(); // Make sure to clear RGB too
        }
        delay(50); 
    }
    last_button_state = current_button_state;

    // 2. Data Capture
    if (logging_active) {
        // --- Get Capacitive Reading ---
        int val = analogRead(A0); 
        capacitive_readings.push_back(val);
        if (capacitive_readings.size() > MAX_READINGS) {
            capacitive_readings.erase(capacitive_readings.begin());
        }
        
        // -- Get Color Reading --
        int r, g, b; 
        
        // Ensure data is ready
        if (APDS.colorAvailable()) {
            APDS.readColor(r, g, b);
            
            float sum = r + g + b;
            
            // Prevent division by zero if sensor sees absolute pitch black
            if (sum > 0) {
                float redRatio = r / sum;
                float greenRatio = g / sum;
                float blueRatio = b / sum;
                
                // CHANGE 3: Store as tuple of floats
                rgb_readings.push_back(std::make_tuple(redRatio, greenRatio, blueRatio));
            } else {
                 rgb_readings.push_back(std::make_tuple(0.0f, 0.0f, 0.0f));
            }

            if (rgb_readings.size() > MAX_READINGS) {
                rgb_readings.erase(rgb_readings.begin());
            }
        }

        // --- Calculate and Print CSV Row ---
        if (capacitive_readings.size() == MAX_READINGS && 
            rgb_readings.size() == MAX_READINGS) {
            
            float avg_cap = calculate_average(capacitive_readings);

            // CHANGE 4: Receive float tuple
            std::tuple<float, float, float> avg_rgb = calculate_tuple_average(rgb_readings);
            float avg_red = std::get<0>(avg_rgb);
            float avg_green = std::get<1>(avg_rgb);
            float avg_blue = std::get<2>(avg_rgb);

            Serial.print(avg_cap, 2); 
            Serial.print(",");
            Serial.print(avg_red, 4);   // Print 4 decimals for better ratio precision
            Serial.print(",");
            Serial.print(avg_green, 4);
            Serial.print(",");
            Serial.println(avg_blue, 4);
        }
    }
    
    delay(50); 
}