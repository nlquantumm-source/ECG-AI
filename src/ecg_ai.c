#include "ecg_ai.h"
#include "model_weights.h" // Include weights ONLY here
#include <math.h>
#include <string.h>

// --- Memory Allocation ---
// Static buffers to prevent stack overflow.
// Size: 32 + 16 + 32 floats = 80 floats * 4 bytes = 320 bytes.
// This fits easily in the 8KB RAM of PIC24FJ64GA002.
static float input_layer[INPUT_SIZE];
static float hidden_layer[HIDDEN_SIZE];
static float output_layer[INPUT_SIZE];

// --- Initialization ---
void ecg_ai_init(void)
{
    // Clear buffers
    memset(input_layer, 0, sizeof(input_layer));
    memset(hidden_layer, 0, sizeof(hidden_layer));
    memset(output_layer, 0, sizeof(output_layer));
}

// --- Helper Functions ---
// ReLU Activation: f(x) = max(0, x)
static float relu(float x) {
    if (x > 0.0f) {
        return x;
    } else {
        return 0.0f;
    }
}

// --- Inference Engine ---
int ecg_ai_run(const int16_t *window, uint16_t len, float *p_normal, float *p_arr)
{
    // Safety check
    if (len != INPUT_SIZE) return -1;

    // 1. Normalize Input
    // Convert 10-bit ADC (0-1023) to Floating Point (0.0-1.0)
    int i, j; // Declare loop vars outside for C89 compatibility if needed
    for (i = 0; i < INPUT_SIZE; i++) {
        input_layer[i] = (float)window[i] / 1023.0f;
    }

    // 2. Encode (Input -> Hidden)
    // Formula: Hidden = ReLU(Input * W1 + B1)
    for (j = 0; j < HIDDEN_SIZE; j++) {
        float sum = B1[j]; // Start with Bias
        for (i = 0; i < INPUT_SIZE; i++) {
            // W1 is flattened: index = i * HIDDEN_SIZE + j
            sum += input_layer[i] * W1[i * HIDDEN_SIZE + j];
        }
        hidden_layer[j] = relu(sum);
    }

    // 3. Decode (Hidden -> Output)
    // Formula: Output = Hidden * W2 + B2
    for (j = 0; j < INPUT_SIZE; j++) {
        float sum = B2[j]; // Start with Bias
        for (i = 0; i < HIDDEN_SIZE; i++) {
            // W2 is flattened: index = i * INPUT_SIZE + j
            sum += hidden_layer[i] * W2[i * INPUT_SIZE + j];
        }
        output_layer[j] = sum;
    }

    // 4. Anomaly Detection (Logic Bridge)
    // Calculate Mean Squared Error (MSE) between Input and Reconstruction.
    // High Error = The AI does not recognize this shape (Arrhythmia).
    float total_error = 0.0f;
    for (i = 0; i < INPUT_SIZE; i++) {
        float diff = input_layer[i] - output_layer[i];
        total_error += (diff * diff);
    }

    // Threshold: If error > 0.05, assume anomaly.
    float anomaly_threshold = 0.05f;

    if (total_error < anomaly_threshold) {
        // Normal
        *p_normal = 0.9f;
        *p_arr = 0.1f;
    } else {
        // Arrhythmia (Anomaly)
        *p_normal = 0.1f;
        *p_arr = 0.9f;
    }

    return 0; // Success
}