#include <stdio.h>
#include "utils.h"

#include <stdlib.h>
#include <math.h>

int16_t max_value = INT16_MIN;
int16_t min_value = INT16_MAX;

float
rms(int16_t *function, int total_samples) {
    float integral_squared = 0.0f, y_rms = 0.0f;

	/* Realiza o somatório das amostras ao quadrado do sinal */
	for (int sample = 0; sample < total_samples; sample++)
	{
		integral_squared += pow(function[sample], 2.0f);
	}

	/* Faz a divisão do somatório pelo número de amostras
	 * e calcula a raíz quadrada
	 */
	y_rms = sqrt((1.0f / total_samples) * integral_squared);

	return y_rms;
}

int16_t
max(int16_t vet[], uint16_t N)
{
    uint16_t i;

    for (i = 1; i < N; i++)
        if (vet[i] > max_value)
            max_value = vet[i];
    return max_value;
}

int16_t
min(int16_t vet[], uint16_t N)
{
    uint16_t i;

    for (i = 1; i < N; i++)
        if (vet[i] < min_value)
            min_value = vet[i];
    return min_value;
}

void
reset_maxmin_values()
{
	max_value = INT16_MIN;
	min_value = INT16_MAX;
}

void
reset_buffer(int16_t* buffer_obj, uint16_t buffer_size)
{
	memset(buffer_obj, 0, buffer_size);
}