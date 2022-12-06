#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "app_tasks.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"

const char * TAG = "mqtt_task";

extern QueueHandle_t accel_queue;
extern QueueHandle_t temp_queue;

void publisher_task(void *ignore)
{
    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = "mqtt://broker.emqx.io",
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_start(client);
    vTaskDelay(1000);

    accel_queue_data_t accel_queue_data;
    temp_queue_data_t temp_queue_data;

    char temperature[10] = {0};
    char accel_rms[10] = {0};

    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    for(;;){
        xTaskDelayUntil(&xLastWakeTime, 60000/portTICK_PERIOD_MS);
        
        xQueuePeek(accel_queue, (void *) &accel_queue_data, 0);
        xQueuePeek(temp_queue, (void *) &temp_queue_data, 0);
        sprintf(temperature, "%.1f", temp_queue_data.temp);
        sprintf(accel_rms, "%.3f", accel_queue_data.rms);
        
        esp_mqtt_client_publish(client, "grupoX/temperatura", temperature, 10, 0, 0);
        esp_mqtt_client_publish(client, "grupoX/rmsvibracao", accel_rms, 10, 0, 0);
        ESP_LOGI(TAG, "Mensagem enviada");
    }
}

void cb_connection_ok(void *pvParameter){
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, 16);

	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

    TaskHandle_t publisher_task_handle;
    xTaskCreate(publisher_task, "publisher_task", 10000, NULL, 3, &publisher_task_handle);
}

void mqtt_task(void *ignore)
{
    wifi_manager_start();
    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, cb_connection_ok);
    for(;;) vTaskDelay(1000);
}
