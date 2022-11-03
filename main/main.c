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
#define MAX_TEXT_SIZE 28

/* Valores instantâneos */
QueueHandle_t accel_queue;
QueueHandle_t temp_queue;

/* Valores Máximos e Mínimos */
QueueHandle_t max_accel_queue;
QueueHandle_t min_accel_queue;

QueueHandle_t max_temp_queue;
QueueHandle_t min_temp_queue;

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
    float current_rms_value;
    int16_t min_accel = INT16_MAX; 
    int16_t max_accel = INT16_MIN;

    for(;;){
        vTaskDelay(10/portTICK_PERIOD_MS);
        mpu6050_accel_read(0, &accel_data);
        for(int i=1; i<MAX_BUFF_SIZE; i++)
            buffer[i] = buffer[i-1];
        
        buffer[0] = accel_data.z;
        current_rms_value = rms(buffer, MAX_BUFF_SIZE);

        xQueuePeek(min_accel_queue, &min_accel, 0);
        xQueuePeek(max_accel_queue, &max_accel, 0);

        min_accel = accel_data.z < min_accel ? accel_data.z : min_accel;
        max_accel = accel_data.z > max_accel ? accel_data.z : max_accel;

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
    float min_temp =  100.0f;
    float max_temp = -100.0f;

    onewire_bus_handle_t owb_handle;
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&owb_config, &owb_handle));
    vTaskDelay(200/portTICK_PERIOD_MS);
    
    // Configurando resolução do sensor de temperatura (DS18B20):
    ESP_ERROR_CHECK(ds18b20_set_resolution(owb_handle, NULL, DS18B20_RESOLUTION_10B));
    vTaskDelay(200/portTICK_PERIOD_MS);

    for(;;)
    {
        vTaskDelay(300/portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(owb_handle, NULL));
        vTaskDelay(200/portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(ds18b20_get_temperature(owb_handle, NULL, &temperature));

        xQueuePeek(min_temp_queue, &min_temp, 0);
        xQueuePeek(max_temp_queue, &max_temp, 0);
        
        min_temp = temperature < min_temp ? temperature : min_temp;
        max_temp = temperature > max_temp ? temperature : max_temp;

        xQueueOverwrite(temp_queue, &temperature);
        xQueueOverwrite(min_temp_queue, &min_temp);
        xQueueOverwrite(max_temp_queue, &max_temp);
    }
}

void
button_task(void* ignore)
{
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);

    int16_t min_accel = INT16_MAX;
    int16_t max_accel = INT16_MIN;
    float min_temp =  100;
    float max_temp = -100;

    for(;;)
    {
        if (gpio_get_level(BUTTON_PIN) == 1)
        {
            xQueueOverwrite(min_accel_queue, &min_accel);
            xQueueOverwrite(max_accel_queue, &max_accel);
            xQueueOverwrite(min_temp_queue, &min_temp);
            xQueueOverwrite(max_temp_queue, &max_temp);
        }
    }
}

void app_main(void)
{
    accel_queue = xQueueCreate(1, sizeof(float));
    temp_queue = xQueueCreate(1, sizeof(float));

    max_accel_queue = xQueueCreate(1, sizeof(int16_t));
    min_accel_queue = xQueueCreate(1, sizeof(int16_t));

    max_temp_queue = xQueueCreate(1, sizeof(float));
    min_temp_queue = xQueueCreate(1, sizeof(float));

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
    float current_temp;
    int16_t current_accel_max, current_accel_min;
    float current_temp_max, current_temp_min;

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

    char text_accel[MAX_TEXT_SIZE], text_temp[MAX_TEXT_SIZE];
    char text_accel_max[MAX_TEXT_SIZE], text_accel_min[MAX_TEXT_SIZE];
    char text_temp_max[MAX_TEXT_SIZE], text_temp_min[MAX_TEXT_SIZE];

    for(;;)
    {
        vTaskDelay(200/portTICK_PERIOD_MS);

        xQueuePeek(accel_queue, (void *) &current_rms, 0);
        xQueuePeek(temp_queue, (void* ) &current_temp, 0);
        xQueuePeek(max_accel_queue, (void* ) &current_accel_max, 0);
        xQueuePeek(min_accel_queue, (void* ) &current_accel_min, 0);
        xQueuePeek(min_temp_queue, &current_temp_min, 0);
        xQueuePeek(max_temp_queue, &current_temp_max, 0);
        
        /* Exibição dos valores no display */

        /* Aceleração */
        sprintf(text_accel, "acc(rms): %.3f g  ", current_rms/MPU6050_ACCEL_LSB_SENS_2G);
        ssd1306_display_text(&oled, 0, text_accel, strlen(text_accel), false);

        sprintf(text_accel_min, "min: % .3f g  ", ((float) current_accel_min)/MPU6050_ACCEL_LSB_SENS_2G);
        ssd1306_display_text(&oled, 1, text_accel_min, strlen(text_accel_min), false); 

        sprintf(text_accel_max, "max: % .3f g  ", ((float) current_accel_max)/MPU6050_ACCEL_LSB_SENS_2G);
        ssd1306_display_text(&oled, 2, text_accel_max, strlen(text_accel_max), false);

        /* Temperatura */
        sprintf(text_temp, "temp: %.1f oC", current_temp);
        ssd1306_display_text(&oled, 5, text_temp, strlen(text_temp), false);

        sprintf(text_temp_min, "min: % .1f oC  ", current_temp_min == 100.0f ? 0.0f : current_temp_min);
        ssd1306_display_text(&oled, 6, text_temp_min, strlen(text_temp_min), false);  

        sprintf(text_temp_max, "max: % .1f oC  ", current_temp_max == -100.0f ? 0.0f : current_temp_max);
        ssd1306_display_text(&oled, 7, text_temp_max, strlen(text_temp_max), false);

    }
}