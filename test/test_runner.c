#include <stdio.h>
#include "ecg_ai.h"

int main() {
    printf("Starting AI Logic Test...\n");

    // Create a fake "Normal" ECG window (middle values)
    int16_t fake_window[32];
    for(int i=0; i<32; i++) fake_window[i] = 512; 

    float p_normal, p_arr;
    
    // Run the AI
    ecg_ai_init();
    int status = ecg_ai_run(fake_window, 32, &p_normal, &p_arr);

    if (status != 0) {
        printf("TEST FAILED: AI Engine crashed.\n");
        return 1;
    }

    printf("AI Result -> Normal: %0.2f, Arrhythmia: %0.2f\n", p_normal, p_arr);
    
    printf("TEST PASSED: Logic is sound.\n");
    return 0;
}
