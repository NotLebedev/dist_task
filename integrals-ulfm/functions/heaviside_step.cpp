#include <cmath>
#include "heaviside_step.h"

extern bool invalidate;

/**
 * Heaviside step function, or the unit step function. 0 for x <= 0, 1 otherwise.
 * This representation calculates it using Fourier series
 *
 * Defines:
 * HS_SERIES_SIZE -- defines number of terms to use in series expansion, default 100
 * HS_PREPARE_COEFFICIENTS -- define to prepare array with precalculated coefficients
 **/
data_t heaviside_step(data_t x) {
#ifndef HS_SERIES_SIZE
#define HS_SERIES_SIZE 100
#endif

#ifdef HS_PREPARE_COEFFICIENTS
    static data_t coeffs[HS_SERIES_SIZE] = {0};
    static bool coeffs_ready = false;
    if (invalidate)
        coeffs_ready = false;
    if (!coeffs_ready) {
        for (int i = 0; i < HS_SERIES_SIZE; i++) {
            coeffs[i] = 2 / ((2 * i + 1) * M_PI);
        }
        coeffs_ready = true;
    }
#endif

    data_t result = 0.5;
    for (int i = 0; i < HS_SERIES_SIZE; i++) {
        int n = 2 * i + 1;
#ifdef HS_PREPARE_COEFFICIENTS
        result += coeffs[i] * sinl(n * x);
#else
        result += 2 / (n * M_PI) * sinl(n * x);
#endif
    }

    return result;
}