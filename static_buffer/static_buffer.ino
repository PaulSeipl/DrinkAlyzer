/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* Includes ---------------------------------------------------------------- */
#include <DrinkAlyzer_inferencing.h>
#include <Arduino_APDS9960.h>
#include <vector>
#include <tuple>
#include <numeric>


// static float features[] = {
//     // copy raw features here (for example from the 'Live classification' page)
//     // see https://docs.edgeimpulse.com/docs/running-your-impulse-arduino
// };

float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
// keep track of where we are in the feature array
size_t feature_ix = 0;

/**
 * @brief      Copy raw feature data in out_ptr
 *             Function called by inference library
 *
 * @param[in]  offset   The offset
 * @param[in]  length   The length
 * @param      out_ptr  The out pointer
 *
 * @return     0
 */
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void print_inference_result(ei_impulse_result_t result);


std::vector<int> capacitive_readings;

// CHANGE 1: Change tuple types from <int, int, int> to <float, float, float>
std::vector<std::tuple<float, float, float>> rgb_readings;

const int MAX_READINGS = 5; 

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


const int ledPin = 2;   

/**
 * @brief      Arduino setup function
 */
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600);
    // comment out the below line to cancel the wait for USB connection (needed for native USB)
    while (!Serial);
    Serial.println("Edge Impulse Inferencing Demo");

    pinMode(ledPin, OUTPUT);
  

    // Setting sensors
     if (!APDS.begin()) {
        Serial.print(F("Error initializing APDS9960 sensor."));
        while (!APDS.begin()) {
        Serial.print(F("."));
        delay(500);
        }
    }
}

/**
 * @brief      Arduino main function
 */
void loop()
{
    ei_printf("Edge Impulse standalone inferencing (Arduino)\n");

    int led_state = digitalRead(ledPin);
    if (led_state == LOW) {
        digitalWrite(ledPin, HIGH);
    }

    // Reading sensor data
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

    if (capacitive_readings.size() == MAX_READINGS && 
        rgb_readings.size() == MAX_READINGS) {
        
        float avg_cap = calculate_average(capacitive_readings);

        // CHANGE 4: Receive float tuple
        std::tuple<float, float, float> avg_rgb = calculate_tuple_average(rgb_readings);
        float avg_red = std::get<0>(avg_rgb);
        float avg_green = std::get<1>(avg_rgb);
        float avg_blue = std::get<2>(avg_rgb);

        features[feature_ix++] = avg_cap;
        features[feature_ix++] = avg_red;
        features[feature_ix++] = avg_green;
        features[feature_ix++] = avg_blue;


        delay(200);

        if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
            ei_printf("The size of your 'features' array is not correct. Expected %lu items, but had %lu\n",
                EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
            delay(1000);
            return;
        }

        ei_impulse_result_t result = { 0 };

        // the features are stored into flash, and we don't want to load everything into RAM
        signal_t features_signal;
        features_signal.total_length = sizeof(features) / sizeof(features[0]);
        features_signal.get_data = &raw_feature_get_data;

        // invoke the impulse
        EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
        if (res != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", res);
            return;
        }

        // print inference return code
        ei_printf("run_classifier returned: %d\r\n", res);
        print_inference_result(result);

        delay(1000);
        }
 
}

void print_inference_result(ei_impulse_result_t result) {

    // Print how long it took to perform inference
    ei_printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n",
            result.timing.dsp,
            result.timing.classification,
            result.timing.anomaly);

    // Print the prediction results (object detection)
#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    ei_printf("Object detection bounding boxes:\r\n");
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value == 0) {
            continue;
        }
        ei_printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
                bb.label,
                bb.value,
                bb.x,
                bb.y,
                bb.width,
                bb.height);
    }

    // Print the prediction results (classification)
#else
    ei_printf("Predictions:\r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
        ei_printf("%.5f\r\n", result.classification[i].value);
    }
#endif

    // Print anomaly result (if it exists)
#if EI_CLASSIFIER_HAS_ANOMALY
    ei_printf("Anomaly prediction: %.3f\r\n", result.anomaly);
#endif

#if EI_CLASSIFIER_HAS_VISUAL_ANOMALY
    ei_printf("Visual anomalies:\r\n");
    for (uint32_t i = 0; i < result.visual_ad_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.visual_ad_grid_cells[i];
        if (bb.value == 0) {
            continue;
        }
        ei_printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
                bb.label,
                bb.value,
                bb.x,
                bb.y,
                bb.width,
                bb.height);
    }
#endif

}