#include <stdio.h>
#include "utils.h"

#include <stdlib.h>
#include <math.h>

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
