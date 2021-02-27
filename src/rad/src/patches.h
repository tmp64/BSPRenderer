#ifndef PATCHES_H
#define PATCHES_H
#include "utils.h"

/**
 * Converts float to int rounding up while checking with EPSILON.
 * Value is >= 1.
 */
inline int texFloatToInt(float flVal) {
    if (flVal < 1 || floatEquals(flVal, 1)) {
        return 1;
    }

    float intpart;
    float fracpart = std::modf(flVal, &intpart);

    if (floatEquals(fracpart, 0)) {
        return (int)intpart;
    }

    return (int)intpart + 1;
}

/**
 * Splits all faces into patches. Puts them into g_Patches.
 */
void createPatches();

/**
 * Calculates view factors between patches in O(n^2). Takes a long time when patch size is small.
 */
void calcViewFactors();

/**
 * Adds sunlight to patches.
 */
void applyEnvLighting();

/**
 * Bounces light around.
 */
void bounceLight();

#endif
