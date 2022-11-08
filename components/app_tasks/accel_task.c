#include <stdio.h>
#include <string.h>
#include "app_tasks.h"
#include "mpu6050.h"
#include "driver/i2c.h"

const int8_t I2C0_MASTER_SDA_IO = 21;
const int8_t I2C0_MASTER_SCL_IO = 22;

// 1000 amostras de 10ms = 10s (Window Size):
#define MAX_BUFF_SIZE 1000

extern QueueHandle_t accel_queue;

void accel_task(void * ignore)
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

    int16_t buffer[MAX_BUFF_SIZE] = {0};

    mpu6050_accel_data accel_data;
    accel_queue_data_t queue_data;
    
    queue_data.min = INT16_MAX; 
    queue_data.max = INT16_MIN;

    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    for(;;){
        vTaskDelayUntil(&xLastWakeTime, 10/portTICK_PERIOD_MS);
        mpu6050_accel_read(&accel_data);

        memcpy(buffer+1, buffer, (MAX_BUFF_SIZE-1)*sizeof(int16_t));
        
        buffer[0] = accel_data.z;
        xQueuePeek(accel_queue, &queue_data, 0);
        queue_data.rms = rms(buffer, MAX_BUFF_SIZE);
        queue_data.min = MIN(accel_data.z, queue_data.min);
        queue_data.max = MAX(accel_data.z, queue_data.max);
        xQueueOverwrite(accel_queue, &queue_data);
    }
}