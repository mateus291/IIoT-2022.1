#include <stdio.h>
#include <string.h>
#include "app_tasks.h"
#include "ssd1306.h"
#include "mpu6050.h"

#define I2C1_MASTER_SDA_IO 32
#define I2C1_MASTER_SCL_IO 33

#define MAX_TEXT_SIZE 28

extern QueueHandle_t accel_queue;
extern QueueHandle_t temp_queue;

void oled_task(void * ignore)
{
    accel_queue_data_t accel_queue_data;
    temp_queue_data_t temp_queue_data;

    float accel_rms, temp, temp_max, temp_min;
    int16_t accel_max, accel_min;

    // Configurando display OLED:
    SSD1306_t oled;
    i2c_master_init(&oled, I2C1_MASTER_SDA_IO, I2C1_MASTER_SCL_IO, -1);
    ssd1306_init(&oled, 128, 64);
    ssd1306_clear_screen(&oled, false);
    ssd1306_contrast(&oled, 0xFF);
    ssd1306_display_text_x3(&oled, 0, "IIoT", 4, false);
    ssd1306_display_text_x3(&oled, 5, " 22.1", 6, false);
    vTaskDelay(2000/portTICK_PERIOD_MS);
    ssd1306_clear_screen(&oled, false);

    char text_accel[MAX_TEXT_SIZE], text_temp[MAX_TEXT_SIZE];
    char text_accel_max[MAX_TEXT_SIZE], text_accel_min[MAX_TEXT_SIZE];
    char text_temp_max[MAX_TEXT_SIZE], text_temp_min[MAX_TEXT_SIZE];

    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    
    for(;;){
        vTaskDelayUntil(&xLastWakeTime, 30/portTICK_PERIOD_MS);
        
        xQueuePeek(accel_queue, (void *) &accel_queue_data, 0);
        xQueuePeek(temp_queue,  (void *) &temp_queue_data , 0);
        
        accel_rms = accel_queue_data.rms; temp = temp_queue_data.temp;
        accel_min = accel_queue_data.min; accel_max = accel_queue_data.max;
        temp_min = temp_queue_data.min; temp_max = temp_queue_data.max;
        
        /* Exibição dos valores no display */

        /* Aceleração */
        sprintf(text_accel, "acc(rms): %.3fg  ", accel_rms/MPU6050_ACCEL_LSB_SENS_2G);
        ssd1306_display_text(&oled, 0, text_accel, strlen(text_accel), false);

        sprintf(text_accel_min, "min: % .3f g  ", ((float) accel_min)/MPU6050_ACCEL_LSB_SENS_2G);
        ssd1306_display_text(&oled, 1, accel_min >= INT16_MAX ? "min:  ----- g " : text_accel_min, 
                                strlen(text_accel_min), false); 

        sprintf(text_accel_max, "max: % .3f g  ", ((float) accel_max)/MPU6050_ACCEL_LSB_SENS_2G);
        ssd1306_display_text(&oled, 2, accel_max <= INT16_MIN ? "max:  ----- g " : text_accel_max,
                                strlen(text_accel_max), false);

        /* Temperatura */
        sprintf(text_temp, "temp: %4.1f oC", temp);
        ssd1306_display_text(&oled, 5, text_temp, strlen(text_temp), false);

        sprintf(text_temp_min, "min: % .1f oC  ", temp_min);
        ssd1306_display_text(&oled, 6, temp_min >= 100.0f ? "min:  ---- oC " : text_temp_min,
                                strlen(text_temp_min), false);  

        sprintf(text_temp_max, "max: % .1f oC  ", temp_max);
        ssd1306_display_text(&oled, 7, temp_max <= -100.0f ? "min:  ---- oC " : text_temp_max, 
                                strlen(text_temp_max), false); 

    }
}