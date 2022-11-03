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

#define I2C1_MASTER_SDA_IO 32
#define I2C1_MASTER_SCL_IO 33

#define ONEWIRE_GPIO_PIN GPIO_NUM_15
#define BUTTON_PIN GPIO_NUM_4

#define MAX_BUFF_SIZE 1000
#define MAX_TEXT_SIZE 28

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

QueueHandle_t accel_queue;
QueueHandle_t temp_queue;

void accel_task(void *ignore)
{
    // Configuração e instalação do MPU6050:
    mpu6050_config_t mpu6050_config = {
        .i2c_port = 0,
        .sda = I2C0_MASTER_SDA_IO,
        .scl = I2C0_MASTER_SCL_IO,
        .scale = MPU6050_ACCEL_FULL_SCALE_2G,
    };
    mpu6050_accel_install(&mpu6050_config);
    vTaskDelay(200/portTICK_PERIOD_MS);

    int16_t buffer[1000] = {0};

    mpu6050_accel_data accel_data;
    accel_queue_data_t queue_data;
    
    queue_data.min = INT16_MAX; 
    queue_data.max = INT16_MIN;

    TickType_t xLastWakeTime;
    vTaskDelay(100/portTICK_PERIOD_MS);

    xLastWakeTime = xTaskGetTickCount();
    for(;;){
        vTaskDelayUntil(&xLastWakeTime, 10/portTICK_PERIOD_MS);
        mpu6050_accel_read(&accel_data);

        for(int i=1; i<MAX_BUFF_SIZE; i++)
            buffer[i] = buffer[i-1];
        
        buffer[0] = accel_data.z;
        xQueuePeek(accel_queue, &queue_data, 0);
        queue_data.rms = rms(buffer, MAX_BUFF_SIZE);
        queue_data.min = MIN(accel_data.z, queue_data.min);
        queue_data.max = MAX(accel_data.z, queue_data.max);
        xQueueOverwrite(accel_queue, (void *) &queue_data);
    }
}

void temp_task(void* ignore)
{
    /* Configuração do sensor de temperatura */
    // Configurando e instalando driver do barramento 1-Wire:
    onewire_rmt_config_t owb_config = {
        .gpio_pin = ONEWIRE_GPIO_PIN,
        .max_rx_bytes = 10, // 10 tx bytes(1byte ROM command + 8byte ROM number + 1byte device command)
    };
    onewire_bus_handle_t owb_handle;
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&owb_config, &owb_handle));
    vTaskDelay(200/portTICK_PERIOD_MS);
    
    // Configurando resolução do sensor de temperatura (DS18B20):
    ESP_ERROR_CHECK(ds18b20_set_resolution(owb_handle, NULL, DS18B20_RESOLUTION_10B));
    vTaskDelay(200/portTICK_PERIOD_MS);

    float temperature;
    temp_queue_data_t queue_data;

    queue_data.min =  100.0f;
    queue_data.max = -100.0f;

    TickType_t xLastWakeTime;
    vTaskDelay(100/portTICK_PERIOD_MS);

    xLastWakeTime = xTaskGetTickCount();
    for(;;){
        vTaskDelayUntil(&xLastWakeTime, 500/portTICK_PERIOD_MS);

        ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(owb_handle, NULL));
        vTaskDelay(200/portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(ds18b20_get_temperature(owb_handle, NULL, &temperature));

        xQueuePeek(temp_queue, &queue_data, 0);
        queue_data.temp = temperature;
        queue_data.min = MIN(temperature, queue_data.min);
        queue_data.max = MAX(temperature, queue_data.max);
        xQueueOverwrite(temp_queue, &queue_data);
    }
}

void button_task(void* ignore)
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

void app_main(void)
{
    accel_queue = xQueueCreate(1, sizeof(accel_queue_data_t));
    temp_queue  = xQueueCreate(1, sizeof(temp_queue_data_t));

    TaskHandle_t accel_task_handle;
    TaskHandle_t temp_task_handle;
    TaskHandle_t button_task_handle;

    xTaskCreate(accel_task, "accel_task", 10000, NULL, 0, &accel_task_handle);
    xTaskCreate(temp_task, "temp_task", 10000, NULL, 0, &temp_task_handle);
    xTaskCreate(button_task, "button_task", 10000, NULL, 0, &button_task_handle);

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
    vTaskDelay(100/portTICK_PERIOD_MS);

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