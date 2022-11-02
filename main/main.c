#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

#include "mpu6050.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include "ssd1306.h"
#include "font8x8_basic.h"

#include <string.h>
#include "mpu6050_task.h"

void app_main(void)
{
    QueueHandle_t accel_queue = xQueueCreate(1, 1000*sizeof(int16_t) );

    TaskHandle_t mpu6050_task_handle;
    xTaskCreate(mpu6050_task, "mpu6050_task", 
                2048, (void *) accel_queue, 2,
                &mpu6050_task_handle);

    int16_t 

    for(;;)
    {

        ESP_LOGI("teste", "%d", );

    }
}