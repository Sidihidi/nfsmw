#include "./sndcmn.h"

float SNDI_cos(float input) {
    while (input > 6.2831855f) {
        input -= 6.2831855f;
    }

    float x2 = input * input;
    float x4 = x2 * x2;
    float x6 = x2 * x4;
    float x8 = x4 * x4;
    float x10 = x2 * x8;
    float x12 = x6 * x6;

    return 1.0f - x2 * 0.5f + x4 * 0.041666668f - x6 * 0.0013888889f +
           x8 * 2.4801587e-05f - x10 * 2.7557320e-07f +
           x12 * 2.0876758e-09f;
}
