#ifndef UTILS_H_LIB
#define UTILS_H_LIB

#include <stdint.h>

#define MIN(a, b) ( (a < b) ? (a) : (b));
#define MAX(a, b) ( (a > b) ? (a) : (b));

float rms(int16_t *y, int numel);

#endif