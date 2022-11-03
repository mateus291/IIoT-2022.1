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
#include "utils.h"

const int8_t I2C0_MASTER_SDA_IO = 21;
const int8_t I2C0_MASTER_SCL_IO = 22;
const int32_t I2C0_MASTER_FREQ_HZ = 400000;

#define I2C1_MASTER_SDA_IO 32
#define I2C1_MASTER_SCL_IO 33

QueueHandle_t accel_queue;

void accel_task(void *ignore)
{
    // Configuração do MPU6050:
    i2c_port_t mpu6050_i2c_port = 0;
    i2c_config_t mpu6050_i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C0_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C0_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C0_MASTER_FREQ_HZ,
        .clk_flags = 0,
    };
    
    ESP_ERROR_CHECK(i2c_param_config(mpu6050_i2c_port, &mpu6050_i2c_config));
	ESP_ERROR_CHECK(i2c_driver_install(mpu6050_i2c_port, I2C_MODE_MASTER, 0, 0, 0));
    vTaskDelay(200/portTICK_PERIOD_MS);

    // Configurando escala de leitura do Acelerômetro (MPU6050):
    mpu6050_accel_config(mpu6050_i2c_port, MPU6050_ACCEL_FULL_SCALE_2G);
    vTaskDelay(200/portTICK_PERIOD_MS);

    mpu6050_accel_data accel_data;

    int16_t buffer[1000] = {0};
    float rms_value;

    for(;;){
        vTaskDelay(10/portTICK_PERIOD_MS);
        mpu6050_accel_read(0, &accel_data);
        for(int i=1; i<1000; i++)
            buffer[i] = buffer[i-1];
        
        buffer[0] = accel_data.z;
        rms_value = rms(buffer, 1000);
        xQueueOverwrite(accel_queue, (void *) &rms_value);
    }
}

void app_main(void)
{
    accel_queue = xQueueCreate(1, sizeof(float));

    TaskHandle_t mpu6050_task_handle;
    xTaskCreate(accel_task, "mpu6050_task", 
                10000, NULL, 0,
                &mpu6050_task_handle);

    float crrt_rms;

    // Configurando display OLED:
    SSD1306_t oled;
    i2c_master_init(&oled, I2C1_MASTER_SDA_IO, I2C1_MASTER_SCL_IO, -1);
    ssd1306_init(&oled, 128, 64);
    ssd1306_clear_screen(&oled, false);
    ssd1306_contrast(&oled, 0xFF);
    ssd1306_display_text_x3(&oled, 0, "TEuKo", 6, false);
    ssd1306_display_text(&oled, 6, "inc.", 4, false);
    vTaskDelay(2000/portTICK_PERIOD_MS);
    ssd1306_clear_screen(&oled, false);

    char text[20];

    for(;;)
    {
        vTaskDelay(200/portTICK_PERIOD_MS);

        xQueuePeek(accel_queue, (void *) &crrt_rms, 0);
        
        sprintf(text, "RMS: %.3f", crrt_rms/MPU6050_ACCEL_LSB_SENS_2G - 1.0f);
        ssd1306_display_text(&oled, 0, text, 20, false);
    }
}