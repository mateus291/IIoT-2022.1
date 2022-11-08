#include <stdio.h>
#include "app_tasks.h"
#include "ds18b20.h"
#include "onewire_bus.h"

#define ONEWIRE_GPIO_PIN GPIO_NUM_15

extern QueueHandle_t temp_queue;

void temp_task(void * ignore)
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
