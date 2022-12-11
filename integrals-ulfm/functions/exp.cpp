#include <cmath>
#include "exp.h"

extern bool invalidate;

/**
 * Exponent function represented by taylor series
 *
 * Defines:
 * EXP_SERIES_SIZE -- defines number of terms to use in series expansion, default 100
 * EXP_PREPARE_COEFFICIENTS -- define to prepare array with precalculated coefficients
 **/
data_t exp(data_t x) {
#ifndef EXP_SERIES_SIZE
#define EXP_SERIES_SIZE 100
#endif

#ifdef EXP_PREPARE_COEFFICIENTS
    static data_t coeffs[EXP_SERIES_SIZE] = {0};
    static bool coeffs_ready = false;
    if (invalidate)
        coeffs_ready = false;
    if (!coeffs_ready) {
        for (int n = 0; n < EXP_SERIES_SIZE; n++) {
            coeffs[n] = 1 / tgammal(n + 1);
        }
        coeffs_ready = true;
    }
#endif

    data_t result = 0.0;
    for (int n = 0; n < EXP_SERIES_SIZE; n++) {
#ifdef EXP_PREPARE_COEFFICIENTS
        result += coeffs[n] * powl(x, n);
#else
        result += powl(x, n) / tgammal(n + 1);
#endif
    }

    return result;
}