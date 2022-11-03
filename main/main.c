#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "mpu6050.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include "ssd1306.h"
#include "font8x8_basic.h"

#include <string.h>
#include "accel_task.h"
#include "utils.h"

#define I2C1_MASTER_SDA_IO 32
#define I2C1_MASTER_SCL_IO 33

void app_main(void)
{
    QueueHandle_t accel_queue = xQueueCreate(1, sizeof(int16_t));

    TaskHandle_t mpu6050_task_handle;
    xTaskCreate(accel_task, "mpu6050_task", 
                10000, (void *) accel_queue, 2,
                &mpu6050_task_handle);

    float crrt_rms;

    // Configurando display OLED:
    SSD1306_t oled;
    i2c_master_init(&oled, I2C1_MASTER_SDA_IO, I2C1_MASTER_SCL_IO, -1);
    ssd1306_init(&oled, 128, 64);
    ssd1306_clear_screen(&oled, false);
    ssd1306_contrast(&oled, 0xFF);
    vTaskDelay(200/portTICK_PERIOD_MS);

    char text[20];

    for(;;)
    {
        vTaskDelay(200/portTICK_PERIOD_MS);

        xQueueReceive(accel_queue, (void *) &crrt_rms, 1000);
        
        sprintf(text, "RMS: %.3f", crrt_rms);
        ssd1306_clear_screen(&oled, false);
        ssd1306_contrast(&oled, 0xFF);
        ssd1306_display_text(&oled, 0, text, 20, false);
    }
}