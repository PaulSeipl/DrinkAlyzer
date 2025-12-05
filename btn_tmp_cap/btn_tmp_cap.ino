#include <DS18B20.h>
#include <vector>
#include <tuple>
#include <numeric>
#include <Arduino_APDS9960.h>

// Initialize DS18B20 on digital pin 2
DS18B20 ds(2);

// Define the pin for the start button
const int START_BUTTON_PIN = 5; 

// Vectors to store the last 5 readings
std::vector<int> capacitive_readings;
std::vector<float> temp_readings;

// Vectors to store RGB values
std::vector<std::tuple<int, int, int>> rgb_readings;

const int MAX_READINGS = 5; // The number of readings to average
bool logging_active = false; // State variable for logging control
bool last_button_state = HIGH; // For basic debouncing (HIGH because of PULLUP)

// --- Averaging Functions (Same as before) ---
float calculate_average(const std::vector<int>& data) {
    if (data.empty()) return 0.0;
    long sum = std::accumulate(data.begin(), data.end(), 0L); 
    return (float)sum / data.size();
}

float calculate_average(const std::vector<float>& data) {
    if (data.empty()) return 0.0f;
    float sum = std::accumulate(data.begin(), data.end(), 0.0f); 
    return sum / data.size();
}

std::tuple<int, int, int> calculate_tuple_average(const std::vector<std::tuple<int, int, int>>& data) {
    if (data.empty()) return std::make_tuple(0, 0, 0);

    // Use long long to prevent overflow if the vector is very large
    long long sum_r = 0;
    long long sum_g = 0;
    long long sum_b = 0;

    for (const auto& item : data) {
        sum_r += std::get<0>(item);
        sum_g += std::get<1>(item);
        sum_b += std::get<2>(item);
    }

    size_t count = data.size();
    
    // Integer division will truncate decimals. 
    return std::make_tuple(
        static_cast<int>(sum_r / count), 
        static_cast<int>(sum_g / count), 
        static_cast<int>(sum_b / count)
    );
}
// ---------------------------------------------

void setup() {
  // Open serial port for communication
  Serial.begin(9600); 
  
  // Set the button pin as an input with a built-in pull-up resistor
  // The button will read LOW when pressed and HIGH when released.
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  
    // initialize color sensor 
    if (!APDS.begin()) {
        // If the sensor is not initialized, throw an error and wait until it is available.
        Serial.print(F("Error initializing APDS9960 sensor."));
        while (!APDS.begin()) {
        Serial.print(F("."));
        delay(500);
        }
    }

  Serial.println("Ready. Press button on D1 to start logging...");
}

void loop() {
    // 1. Check Button State (Toggle Logging)
    int current_button_state = digitalRead(START_BUTTON_PIN);
    logging_active = true;
    // Basic debouncing and state change detection (Falling edge: HIGH to LOW)
    if (current_button_state == LOW && last_button_state == HIGH) {
        // Button was just pressed (LOW)
        logging_active = true; // !logging_active; // Toggle the state
        
        if (logging_active) {
            // Print the CSV header when logging starts
            Serial.println();
            Serial.println("--- LOGGING STARTED ---");
            Serial.println("Timestamp,Avg_Capacitive,Avg_Temp_C");
        } else {
            Serial.println("--- LOGGING STOPPED ---");
            // Clear lists when stopping to reset the averaging buffer
            capacitive_readings.clear();
            temp_readings.clear();
        }
        
        // Short delay to handle switch bounce (basic debounce)
        delay(50); 
    }
    // Update the last state for the next check
    last_button_state = current_button_state;


    // 2. Data Capture and Logging Logic
    if (logging_active) {
        // --- Get Capacitive Reading ---
        int val = analogRead(A0); 
        capacitive_readings.push_back(val);
        if (capacitive_readings.size() > MAX_READINGS) {
            capacitive_readings.erase(capacitive_readings.begin());
        }

        // --- Get Temperature Reading ---
        float tempC = -999.0;
        while (ds.selectNext()) {
            Serial.println("I am in the while");
            tempC = ds.getTempC();
            temp_readings.push_back(tempC);
            if (temp_readings.size() > MAX_READINGS) {
                temp_readings.erase(temp_readings.begin());
            }
        }
        
        // -- Get Color Reading --
        int r, g, b; 
        float sum;
        while (!APDS.colorAvailable() || !APDS.proximityAvailable()) {}
        APDS.readColor(r, g, b);
        float redRatio = r / sum;
        float greenRatio = g / sum;
        float blueRatio = b / sum;
        std::tuple<int, int, int> rgb = std::make_tuple(redRatio, greenRatio, blueRatio);
        rgb_readings.push_back(rgb);
        if (rgb_readings.size() > MAX_READINGS) {
            rgb_readings.erase(rgb_readings.begin());
        }


        // --- Calculate and Print CSV Row ---
        // Only print data once we have MAX_READINGS for both
        if (capacitive_readings.size() == MAX_READINGS && temp_readings.size() == MAX_READINGS && rgb_readings.size() == MAX_READINGS) {
            
            float avg_cap = calculate_average(capacitive_readings);
            float avg_temp = calculate_average(temp_readings);

            std::tuple<int, int, int> avg_rgb = calculate_tuple_average(rgb_readings);
            float avg_red = std::get<0>(avg_rgb);
            float avg_green = std::get<1>(avg_rgb);
            float avg_blue = std::get<2>(avg_rgb);

            // Format the output as a CSV row: Avg_Capacitive,Avg_Temp_C
            // We use millis() for a simple timestamp
            Serial.print(millis());
            Serial.print(",");
            Serial.print(avg_cap, 2); // Print capacitive with 2 decimal places
            Serial.print(",");
            Serial.println(avg_temp, 2); // Print temperature with 2 decimal places

            Serial.println(avg_red, 2);
            Serial.print(",");
            Serial.println(avg_green, 2);
            Serial.print(",");
            Serial.println(avg_blue, 2);
        }
        else {
            Serial.println("Stuck here");
            Serial.println(capacitive_readings.size());
            Serial.println(temp_readings.size());
            Serial.println(rgb_readings.size());
        }
    }
    
    // Read every 100 milliseconds (adjust as needed)
    delay(100); 
}