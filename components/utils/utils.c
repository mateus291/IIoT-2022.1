#include <stdio.h>
#include "utils.h"

#include <stdlib.h>
#include <math.h>

float rms(int16_t *function, int total_samples) {
    float integral_squared = 0.0f, y_rms = 0.0f;

    int32_t sum = 0; // Soma do quadrado das amostras
    float y_rms = 0; // Valor RMS do sinal y
    
    // Percorre o array de amostras y (sinal), acumulando o
    // quadrado de cada amostra em sum:
    for(int i = 0; i < (int) numel; i++)
        sum = sum + y[i]*y[i];
    
    // Divide a soma do quadrado das amostras pelo número de amostras
    // e extrai a raiz quadrada do resultado, guardando o valor em y_rms:
    y_rms = sqrt(((float) sum)/((float) numel));
    
    return y_rms; // Retorna o valor RMS do sinal y.
}

int16_t max(int16_t vet[], uint16_t N){
    uint16_t i;
    int16_t max = vet[0];

    for (i = 1; i < N; i++)
        if (vet[i] > max)
            max = vet[i];
    return max;
}

int16_t min(int16_t vet[], uint16_t N){
    uint16_t i;
    int16_t min = vet[0];

    for (i = 1; i < N; i++)
        if (vet[i] < min)
            min = vet[i];
    return min;
