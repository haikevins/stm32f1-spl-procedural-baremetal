#ifndef IMU_TYPES_H
#define IMU_TYPES_H

#include <stdint.h>

typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t temperature;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} imu_raw_sample_t;

typedef struct
{
    imu_raw_sample_t raw;

    int32_t accel_x_mg;
    int32_t accel_y_mg;
    int32_t accel_z_mg;

    int32_t temperature_millidegrees_c;

    int32_t gyro_x_millidegrees_per_second;
    int32_t gyro_y_millidegrees_per_second;
    int32_t gyro_z_millidegrees_per_second;
} imu_sample_t;

typedef enum
{
    IMU_DEVICE_UNKNOWN = 0x00,
    IMU_DEVICE_MPU6500 = 0x70,
    IMU_DEVICE_MPU9250_COMPATIBLE = 0x71
} imu_device_t;

#endif /* IMU_TYPES_H */
