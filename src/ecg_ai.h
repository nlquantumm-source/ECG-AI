#ifndef ECG_AI_H
#define ECG_AI_H

#include <stdint.h>

// Must match the Input Size in model_weights.h
#define ECG_WINDOW_SIZE 32

// Initializes the AI engine
void ecg_ai_init(void);

// Runs the inference using the Raw C weights
// Returns 0 on success
// Puts probabilities into p_normal and p_arr
int ecg_ai_run(const int16_t *window, uint16_t len, float *p_normal, float *p_arr);

#endif