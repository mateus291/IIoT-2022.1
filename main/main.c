#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

#include "mpu6050.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include "ssd1306.h"
#include "font8x8_basic.h"

// Configuração do I2C0 (Conectado ao MPU6050):
const uint8_t I2C0_MASTER_SDA_IO = GPIO_NUM_21;
const uint8_t I2C0_MASTER_SCL_IO = GPIO_NUM_22;
const uint32_t I2C0_MASTER_FREQ_HZ = 250000;

// Configuração do I2C1 (Conectado ao OLED 128x64):
const uint8_t I2C1_MASTER_SDA_IO = GPIO_NUM_32;
const uint8_t I2C1_MASTER_SCL_IO = GPIO_NUM_33;

#define ONEWIRE_GPIO_PIN GPIO_NUM_15

void app_main(void)
{
    // Configurando e instalando driver do barramento I2C0 (MPU6050):
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

    // Configurando display OLED:
    SSD1306_t oled;
    i2c_master_init(&oled, I2C1_MASTER_SDA_IO, I2C1_MASTER_SCL_IO, -1);
    ssd1306_init(&oled, 128, 64);
    ssd1306_clear_screen(&oled, false);
    ssd1306_contrast(&oled, 0xFF);
    vTaskDelay(200/portTICK_PERIOD_MS);

    mpu6050_accel_data accel_data;
    float x, y, z;

    float temperature;

    for(;;){
        mpu6050_accel_read(mpu6050_i2c_port, &accel_data);

        x = ((float) accel_data.x) / MPU6050_ACCEL_LSB_SENS_2G;
        y = ((float) accel_data.y) / MPU6050_ACCEL_LSB_SENS_2G;
        z = ((float) accel_data.z) / MPU6050_ACCEL_LSB_SENS_2G;

        ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(owb_handle, NULL));
        vTaskDelay(200/portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(ds18b20_get_temperature(owb_handle, NULL, &temperature));

        char temp_text[50];
        sprintf(temp_text, "Temp.: %.1f oC\n", temperature);

        char x_text[30], y_text[30], z_text[30];

        sprintf(x_text, "x: %.3f\n", x);
        sprintf(y_text, "y: %.3f\n", y);
        sprintf(z_text, "z: %.3f\n", z);

        ssd1306_clear_screen(&oled, false);
        ssd1306_contrast(&oled, 0xFF);
        ssd1306_display_text(&oled, 0, temp_text, 14, false);
        ssd1306_display_text(&oled, 3, "Accel.:", 7, false);
        ssd1306_display_text(&oled, 5, x_text, 10, false);
        ssd1306_display_text(&oled, 6, y_text, 10, false);
        ssd1306_display_text(&oled, 7, z_text, 10, false);

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}