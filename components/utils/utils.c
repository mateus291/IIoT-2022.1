#include <stdio.h>
#include "utils.h"

#include <stdlib.h>
#include <math.h>

float rms(int16_t *y, size_t numel) {
    // Mateus Soares Marques 118210035

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