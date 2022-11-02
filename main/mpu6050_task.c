#include "mpu6050.h"
#include "FreeRTOS.h"


// Configuração do I2C0 (Conectado ao MPU6050):
const uint8_t I2C0_MASTER_SDA_IO = GPIO_NUM_21;
const uint8_t I2C0_MASTER_SCL_IO = GPIO_NUM_22;
const uint32_t I2C0_MASTER_FREQ_HZ = 250000;

void mpu6050_config()
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
}

void mpu6050_task(void *pvData)
{
    QueueHandle_t queue = (QueueHandle_t) pvData;

    mpu6050_config();
    int16_t buffer[1000] = {0};
    int16_t trash[1000] = {0};

    for(;;){
        mpu6050_accel_read(0, &accel_data);
        for(int i=1; i<1000; i++)
            buffer[i] = buffer[i-1];
        
        buffer[0] = accel_data.z;

        if(!uxQueueSpacesAvailable(queue))
            xQueueReceive(queue, trash, 1000);

        xQueueSend(queue, buffer, 1000);
    }
}
