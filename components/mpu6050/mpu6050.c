#include "mpu6050.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

void mpu6050_accel_config(i2c_port_t i2c_port, uint8_t resolution)
{
    i2c_cmd_handle_t cmd;

	cmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (MPU6050_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
	i2c_master_write_byte(cmd, MPU6050_ACCEL_XOUT_H, 1);
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
	i2c_master_cmd_begin(i2c_port, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (MPU6050_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
	i2c_master_write_byte(cmd, MPU6050_PWR_MGMT_1, 1);
	i2c_master_write_byte(cmd, 0, 1);
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
	i2c_master_cmd_begin(i2c_port, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (MPU6050_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
	i2c_master_write_byte(cmd, MPU6050_ACCEL_CONFIG, 1);
	i2c_master_write_byte(cmd, resolution, 1);
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
	i2c_master_cmd_begin(i2c_port, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
}

void mpu6050_accel_read(i2c_port_t i2c_port, mpu6050_accel_data *accel_data)
{
    i2c_cmd_handle_t cmd;

    uint8_t data[6];

	cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (MPU6050_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, MPU6050_ACCEL_XOUT_H, 1));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    ESP_ERROR_CHECK(i2c_master_cmd_begin(i2c_port, cmd, 1000/portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (MPU6050_I2C_ADDRESS << 1) | I2C_MASTER_READ, 1));

	ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data,   0));
	ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+1, 0));
	ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+2, 0));
	ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+3, 0));
	ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+4, 0));
	ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+5, 1));
    
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
    ESP_ERROR_CHECK(i2c_master_cmd_begin(i2c_port, cmd, 1000/portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);

    accel_data -> x = (data[0] << 8) | data[1];
    accel_data -> y = (data[2] << 8) | data[3];
    accel_data -> z = (data[4] << 8) | data[5];
}