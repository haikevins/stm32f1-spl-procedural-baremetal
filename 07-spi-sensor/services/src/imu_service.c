#include "imu_service.h"

#include <stddef.h>

#include "mpu6500.h"

#define MPU6500_ACCEL_LSB_PER_G       (16384L)
#define MPU6500_GYRO_LSB_PER_DPS      (131L)
#define MPU6500_TEMP_SENSITIVITY      (334L)
#define MPU6500_TEMP_OFFSET_MDEG_C    (21000L)

static uint8_t s_who_am_i;
static imu_device_t s_device;

static int32_t imu_service_scale_signed(
    int16_t raw_value,
    int32_t numerator,
    int32_t denominator)
{
    int32_t scaled =
        (int32_t)raw_value * numerator;

    if (scaled >= 0)
    {
        scaled += denominator / 2;
    }
    else
    {
        scaled -= denominator / 2;
    }

    return scaled / denominator;
}

bool imu_service_init(void)
{
    s_who_am_i = 0U;
    s_device = IMU_DEVICE_UNKNOWN;

    if (!mpu6500_init(&s_who_am_i))
    {
        return false;
    }

    if (s_who_am_i == MPU6500_WHO_AM_I_VALUE)
    {
        s_device = IMU_DEVICE_MPU6500;
    }
    else if (s_who_am_i == MPU9250_WHO_AM_I_VALUE)
    {
        s_device = IMU_DEVICE_MPU9250_COMPATIBLE;
    }
    else
    {
        return false;
    }

    return true;
}

bool imu_service_read(imu_sample_t *sample)
{
    imu_raw_sample_t raw;

    if (sample == NULL)
    {
        return false;
    }

    if (!mpu6500_read_raw(&raw))
    {
        return false;
    }

    sample->raw = raw;

    sample->accel_x_mg =
        imu_service_scale_signed(
            raw.accel_x,
            1000L,
            MPU6500_ACCEL_LSB_PER_G);
    sample->accel_y_mg =
        imu_service_scale_signed(
            raw.accel_y,
            1000L,
            MPU6500_ACCEL_LSB_PER_G);
    sample->accel_z_mg =
        imu_service_scale_signed(
            raw.accel_z,
            1000L,
            MPU6500_ACCEL_LSB_PER_G);

    sample->temperature_millidegrees_c =
        MPU6500_TEMP_OFFSET_MDEG_C +
        imu_service_scale_signed(
            raw.temperature,
            1000L,
            MPU6500_TEMP_SENSITIVITY);

    sample->gyro_x_millidegrees_per_second =
        imu_service_scale_signed(
            raw.gyro_x,
            1000L,
            MPU6500_GYRO_LSB_PER_DPS);
    sample->gyro_y_millidegrees_per_second =
        imu_service_scale_signed(
            raw.gyro_y,
            1000L,
            MPU6500_GYRO_LSB_PER_DPS);
    sample->gyro_z_millidegrees_per_second =
        imu_service_scale_signed(
            raw.gyro_z,
            1000L,
            MPU6500_GYRO_LSB_PER_DPS);

    return true;
}

imu_device_t imu_service_get_device(void)
{
    return s_device;
}

uint8_t imu_service_get_who_am_i(void)
{
    return s_who_am_i;
}
