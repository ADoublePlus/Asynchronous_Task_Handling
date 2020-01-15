#pragma once

#include <stdlib.h>

#define SQU(X) (X) * (X)

/*
    random - Return a random number in the range of 0 - 1.

    return float - Returns the random number as a float.
 */
inline float random() { return rand() / (float)RAND_MAX; }

/* 
    randomRange - Returns a random value between the minimum (inclusive) and maximum (exclusive).

    param[in] p_Min - The first value of type T used as the minimum.
    param[in] p_Max - The second value of type T used as the maximum.

    return T - Returns a random value between p_Min and p_Max.
 */
template <typename T>
inline T randomRange(T p_Min, T p_Max) { return (T)(random() * (float)(p_Max - p_Min) + (float)p_Min); }