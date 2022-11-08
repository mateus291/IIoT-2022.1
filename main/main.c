#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "app_tasks.h"

QueueHandle_t accel_queue;
QueueHandle_t temp_queue;

void app_main(void)
{
    accel_queue = xQueueCreate(1, sizeof(accel_queue_data_t));
    temp_queue  = xQueueCreate(1, sizeof(temp_queue_data_t));

    TaskHandle_t accel_task_handle;
    TaskHandle_t temp_task_handle;
    TaskHandle_t button_task_handle;
    TaskHandle_t oled_task_handle;

    xTaskCreate(accel_task, "accel_task", 10000, NULL, 3, &accel_task_handle);
    xTaskCreate(temp_task, "temp_task", 10000, NULL, 3, &temp_task_handle);
    xTaskCreate(button_task, "button_task", 10000, NULL, 1, &button_task_handle);
    xTaskCreate(oled_task, "oled_task", 10000, NULL, 2, &oled_task_handle);
}