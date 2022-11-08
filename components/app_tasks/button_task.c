#include <stdio.h>
#include "app_tasks.h"
#include "driver/gpio.h"

#define BUTTON_PIN GPIO_NUM_4

extern QueueHandle_t accel_queue;
extern QueueHandle_t temp_queue;

void button_task(void * ignore)
{
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);

    accel_queue_data_t accel_queue_data = {
        .rms = 0,
        .min = INT16_MAX,
        .max = INT16_MIN,
    };
    temp_queue_data_t temp_queue_data = {
        .temp = 0.0f,
        .min =  100.0f,
        .max = -100.0f,
    };

    for(;;)
    {
        if (gpio_get_level(BUTTON_PIN) == 1)
        {
            xQueueOverwrite(accel_queue, &accel_queue_data);
            xQueueOverwrite(temp_queue, &temp_queue_data);
        }
    }
}