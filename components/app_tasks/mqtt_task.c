#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "app_tasks.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "mpu6050.h"

const char * TAG = "mqtt_task";

extern QueueHandle_t accel_queue;
extern QueueHandle_t temp_queue;

extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");

static void publisher_task(void *ignore)
{
    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = "mqtts://vibsensor:iiot20221@827677fef74a4840be0e4364cff5581f.s2.eu.hivemq.cloud:8883",
        .broker.verification.certificate = (const char *)mqtt_eclipseprojects_io_pem_start, 
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_start(client);
    vTaskDelay(1000);

    accel_queue_data_t accel_queue_data;
    temp_queue_data_t temp_queue_data;

    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    settimeofday(&now, NULL);

    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    for(;;){
        xTaskDelayUntil(&xLastWakeTime, 30000/portTICK_PERIOD_MS);
        
        xQueuePeek(accel_queue, (void *) &accel_queue_data, 0);
        xQueuePeek(temp_queue, (void *) &temp_queue_data, 0);

        time(&now);

        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);

        char datastr[255] = {0};
        char * meta = "\"meta\":{\"f\":\"100Hz\",\"u\":\"g, C\",\"disp\":\"ds18b20, mpu6050\"}}";
        sprintf(datastr,
            "{\"data\":{\"rms\":\"%.3f\",\"temp\":\"%.1f\",\"time\":\"%s\"},%s",
            accel_queue_data.rms/MPU6050_ACCEL_LSB_SENS_2G,temp_queue_data.temp,strftime_buf,meta);
        esp_mqtt_client_publish(client, "sensorDataIn", datastr, 0, 0, 0);
        if(accel_queue_data.rms >= 2)
            esp_mqtt_client_publish(client, "warning", "max rms!", 0, 0, 0);
        ESP_LOGI(TAG, "%s",datastr);
    }
}

static void cb_connection_ok(void *pvParameter){
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
