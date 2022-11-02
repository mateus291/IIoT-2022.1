#ifndef MPU6050_H
#define MPU6050_H

#include <stdio.h>
#include "esp_err.h"
#include "driver/i2c.h"

#define MPU6050_I2C_ADDRESS     0x68
#define MPU6050_PWR_MGMT_1      0x6B
#define MPU6050_ACCEL_CONFIG    0x1C
#define MPU6050_ACCEL_XOUT_H    0x3B

#define MPU6050_ACCEL_FULL_SCALE_2G     0x00
#define MPU6050_ACCEL_FULL_SCALE_4G     0x08
#define MPU6050_ACCEL_FULL_SCALE_8G     0x10
#define MPU6050_ACCEL_FULL_SCALE_16G    0x18

#define MPU6050_ACCEL_LSB_SENS_2G       16384
#define MPU6050_ACCEL_LSB_SENS_4G       8192
#define MPU6050_ACCEL_LSB_SENS_8G       4096
#define MPU6050_ACCEL_LSB_SENS_16G      2048

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} mpu6050_accel_data;

void mpu6050_accel_config(i2c_port_t i2c_port, uint8_t resolution);
/**
 * @brief Configura a resolução do acelerômetro;
*/

void mpu6050_accel_read(i2c_port_t i2c_port, mpu6050_accel_data * accel_data);
/**
 * @brief Lê dados do acelerômetro;
*/

#endif