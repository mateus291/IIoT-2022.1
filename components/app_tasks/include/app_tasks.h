#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "utils.h"

typedef struct
{
    float rms;
    int16_t min;
    int16_t max;
} accel_queue_data_t;

typedef struct
{
    float temp;
    float min;
    float max;
} temp_queue_data_t;

void accel_task(void * ignore);
void temp_task(void * ignore);
void button_task(void * ignore);
void oled_task(void * ignore);