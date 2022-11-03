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

#define ONEWIRE_GPIO_PIN GPIO_NUM_15
#define BUTTON_PIN GPIO_NUM_4

#define MAX_BUFF_SIZE 1000
#define MAX_TEXT_SIZE 20

/* Valores instantâneos */
QueueHandle_t accel_queue;
QueueHandle_t temper_queue;

/* Valores Máximos e Mínimos */
QueueHandle_t max_accel_queue;
QueueHandle_t min_accel_queue;

QueueHandle_t max_temper_queue;
QueueHandle_t min_temper_queue;

int16_t buffer[MAX_BUFF_SIZE] = {0};

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

    //int16_t buffer[1000] = {0};
    float current_rms_value;
    float min_accel, max_accel;

    for(;;){
        vTaskDelay(10/portTICK_PERIOD_MS);
        mpu6050_accel_read(0, &accel_data);
        for(int i=1; i<MAX_BUFF_SIZE; i++)
            buffer[i] = buffer[i-1];
        
        buffer[0] = accel_data.z;
        current_rms_value = rms(buffer, MAX_BUFF_SIZE);
        min_accel = (float) min(buffer, MAX_BUFF_SIZE);
        max_accel = (float) max(buffer, MAX_BUFF_SIZE);

        ESP_LOGI("MIN/MAX", "Min: %d / Max: %d", (int) min_accel, (int) max_accel);

        xQueueOverwrite(accel_queue, (void *) &current_rms_value);
        xQueueOverwrite(min_accel_queue, (void *) &min_accel);
        xQueueOverwrite(max_accel_queue, (void *) &max_accel);
    }
}

void temper_task(void* ignore)
{
    /* Configuração do sensor de temperatura */
    // Configurando e instalando driver do barramento 1-Wire:
    onewire_rmt_config_t owb_config = {
        .gpio_pin = ONEWIRE_GPIO_PIN,
        .max_rx_bytes = 10, // 10 tx bytes(1byte ROM command + 8byte ROM number + 1byte device command)
    };

    float temperature;

    onewire_bus_handle_t owb_handle;
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&owb_config, &owb_handle));
    vTaskDelay(200/portTICK_PERIOD_MS);
    
    // Configurando resolução do sensor de temperatura (DS18B20):
    ESP_ERROR_CHECK(ds18b20_set_resolution(owb_handle, NULL, DS18B20_RESOLUTION_10B));
    vTaskDelay(200/portTICK_PERIOD_MS);

    for(;;)
    {
        ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(owb_handle, NULL));
        vTaskDelay(200/portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(ds18b20_get_temperature(owb_handle, NULL, &temperature));

        xQueueOverwrite(temper_queue, &temperature);
    }
}

void
button_task(void* ignore)
{
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);

    for(;;)
    {
        if (gpio_get_level(BUTTON_PIN) == 1)
        {
            reset_maxmin_values();
            reset_buffer(buffer, MAX_BUFF_SIZE);
        }
    }
}

void app_main(void)
{
    accel_queue = xQueueCreate(1, sizeof(float));
    temper_queue = xQueueCreate(1, sizeof(float));

    max_accel_queue = xQueueCreate(1, sizeof(float));
    min_accel_queue = xQueueCreate(1, sizeof(float));

    max_temper_queue = xQueueCreate(1, sizeof(float));
    min_temper_queue = xQueueCreate(1, sizeof(float));

    TaskHandle_t mpu6050_task_handle;
    TaskHandle_t ds18b20_task_handle;
    TaskHandle_t button_task_handle;

    xTaskCreate(accel_task, "mpu6050_task", 
                10000, NULL, 0,
                &mpu6050_task_handle);
    xTaskCreate(temper_task, "ds18b20_task",
                10000, NULL, 0,
                &ds18b20_task_handle);
    xTaskCreate(button_task, "button_reset_max_min",
                10000, NULL, 0,
                &button_task_handle);

    float current_rms;
    float current_temper;
    float current_accel_max, current_accel_min;

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

    char text_accel[MAX_TEXT_SIZE], text_temper[MAX_TEXT_SIZE];
    char text_accel_max[MAX_TEXT_SIZE], text_accel_min[MAX_TEXT_SIZE];

    for(;;)
    {
        vTaskDelay(200/portTICK_PERIOD_MS);

        xQueuePeek(accel_queue, (void *) &current_rms, 0);
        xQueuePeek(temper_queue, (void* ) &current_temper, 0);
        xQueuePeek(max_accel_queue, (void* ) &current_accel_max, 0);
        xQueuePeek(min_accel_queue, (void* ) &current_accel_min, 0);
        
        /* Exibição dos valores no display */

        /* Aceleração */
        sprintf(text_accel, "RMS_acc: %.3f g  ", current_rms/MPU6050_ACCEL_LSB_SENS_2G);
        ssd1306_display_text(&oled, 0, text_accel, strlen(text_accel), false);

        sprintf(text_accel_max, "MAX_acc: %.3f g  ", current_accel_max/MPU6050_ACCEL_LSB_SENS_2G);
        ssd1306_display_text(&oled, 1, text_accel_max, strlen(text_accel_max), false);

        sprintf(text_accel_min, "MIN_acc: %.3f g  ", current_accel_min/MPU6050_ACCEL_LSB_SENS_2G);
        ssd1306_display_text(&oled, 2, text_accel_min, strlen(text_accel_min), false); 

        /* Temperatura */
        sprintf(text_temper, "Temp: %.1f oC", current_temper);
        ssd1306_display_text(&oled, 4, text_temper, strlen(text_temper), false);        
    }
}