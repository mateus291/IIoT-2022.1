#ifndef UTILS_H_LIB
#define UTILS_H_LIB

#include <stdint.h>
#include <string.h>

extern int16_t max_value;
extern int16_t min_value;

float rms(int16_t *y, int numel);
int16_t max(int16_t vet[], uint16_t N);
int16_t min(int16_t vet[], uint16_t N);

void reset_maxmin_values(void);
void reset_buffer(int16_t* buffer_obj, uint16_t buffer_size);

#endif