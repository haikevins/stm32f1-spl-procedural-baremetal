#include "mpu6500.h"

#include <stddef.h>

#include "board_sensor_bus.h"

#define MPU6500_SPI_READ_BIT          (0x80U)

#define MPU6500_REG_SMPLRT_DIV        (0x19U)
#define MPU6500_REG_CONFIG            (0x1AU)
#define MPU6500_REG_GYRO_CONFIG       (0x1BU)
#define MPU6500_REG_ACCEL_CONFIG      (0x1CU)
#define MPU6500_REG_ACCEL_CONFIG_2    (0x1DU)
#define MPU6500_REG_ACCEL_XOUT_H      (0x3BU)
#define MPU6500_REG_USER_CTRL         (0x6AU)
#define MPU6500_REG_PWR_MGMT_1        (0x6BU)
#define MPU6500_REG_PWR_MGMT_2        (0x6CU)
#define MPU6500_REG_WHO_AM_I          (0x75U)

#define MPU6500_PWR_DEVICE_RESET      (0x80U)
#define MPU6500_PWR_CLOCK_PLL_XGYRO   (0x01U)
#define MPU6500_USER_I2C_IF_DIS       (0x10U)

#define MPU6500_CONFIG_DLPF_41HZ      (0x03U)
#define MPU6500_ACCEL_DLPF_41HZ       (0x03U)
#define MPU6500_SAMPLE_RATE_DIVIDER   (9U)

#define MPU6500_RESET_DELAY_MS        (100UL)
#define MPU6500_STARTUP_DELAY_MS      (10UL)

static int16_t mpu6500_decode_signed(
    uint8_t most_significant,
    uint8_t least_significant)
{
    return (int16_t)(
        ((uint16_t)most_significant << 8U) |
        (uint16_t)least_significant);
}

static bool mpu6500_write_register(
    uint8_t address,
    uint8_t value)
{
    const uint8_t transfer[2] =
    {
        (uint8_t)(address & ~MPU6500_SPI_READ_BIT),
        value
    };
    bool successful;

    board_sensor_bus_select();
    successful = board_sensor_bus_transfer(
        transfer,
        NULL,
        sizeof(transfer));
    board_sensor_bus_deselect();

    return successful;
}

static bool mpu6500_read_registers(
    uint8_t start_address,
    uint8_t *data,
    size_t length)
{
    const uint8_t address =
        (uint8_t)(start_address | MPU6500_SPI_READ_BIT);
    bool successful;

    if ((data == NULL) && (length != 0U))
    {
        return false;
    }

    board_sensor_bus_select();

    successful = board_sensor_bus_transfer(
        &address,
        NULL,
        1U);

    if (successful)
    {
        successful = board_sensor_bus_transfer(
            NULL,
            data,
            length);
    }

    board_sensor_bus_deselect();

    return successful;
}

static bool mpu6500_read_register(
    uint8_t address,
    uint8_t *value)
{
    return mpu6500_read_registers(
        address,
        value,
        1U);
}

bool mpu6500_init(uint8_t *who_am_i)
{
    uint8_t identity = 0U;

    if (who_am_i == NULL)
    {
        return false;
    }

    board_sensor_bus_deselect();
    board_sensor_bus_delay_ms(MPU6500_STARTUP_DELAY_MS);

    /*
     * Select the SPI host interface immediately after power-up. A device
     * reset clears USER_CTRL, so the I2C interface must be disabled again
     * after the reset delay.
     */
    if (!mpu6500_write_register(
            MPU6500_REG_USER_CTRL,
            MPU6500_USER_I2C_IF_DIS))
    {
        return false;
    }

    if (!mpu6500_write_register(
            MPU6500_REG_PWR_MGMT_1,
            MPU6500_PWR_DEVICE_RESET))
    {
        return false;
    }

    board_sensor_bus_delay_ms(MPU6500_RESET_DELAY_MS);

    if (!mpu6500_write_register(
            MPU6500_REG_USER_CTRL,
            MPU6500_USER_I2C_IF_DIS))
    {
        return false;
    }

    board_sensor_bus_delay_ms(1U);

    if (!mpu6500_read_register(
            MPU6500_REG_WHO_AM_I,
            &identity))
    {
        return false;
    }

    *who_am_i = identity;

    /*
     * GY-91 boards are sold with both MPU6500-class and MPU9250 devices.
     * The accelerometer, gyro, temperature and host-SPI register path used
     * by this example is compatible with WHO_AM_I values 0x70 and 0x71.
     */
    if ((identity != MPU6500_WHO_AM_I_VALUE) &&
        (identity != MPU9250_WHO_AM_I_VALUE))
    {
        return false;
    }

    if (!mpu6500_write_register(
            MPU6500_REG_PWR_MGMT_1,
            MPU6500_PWR_CLOCK_PLL_XGYRO))
    {
        return false;
    }

    if (!mpu6500_write_register(
            MPU6500_REG_PWR_MGMT_2,
            0x00U))
    {
        return false;
    }

    if (!mpu6500_write_register(
            MPU6500_REG_CONFIG,
            MPU6500_CONFIG_DLPF_41HZ))
    {
        return false;
    }

    if (!mpu6500_write_register(
            MPU6500_REG_SMPLRT_DIV,
            MPU6500_SAMPLE_RATE_DIVIDER))
    {
        return false;
    }

    /*
     * GYRO_CONFIG = 0 selects +/-250 degrees/s.
     * ACCEL_CONFIG = 0 selects +/-2 g.
     */
    if (!mpu6500_write_register(
            MPU6500_REG_GYRO_CONFIG,
            0x00U))
    {
        return false;
    }

    if (!mpu6500_write_register(
            MPU6500_REG_ACCEL_CONFIG,
            0x00U))
    {
        return false;
    }

    if (!mpu6500_write_register(
            MPU6500_REG_ACCEL_CONFIG_2,
            MPU6500_ACCEL_DLPF_41HZ))
    {
        return false;
    }

    board_sensor_bus_delay_ms(MPU6500_STARTUP_DELAY_MS);

    return true;
}

bool mpu6500_read_raw(imu_raw_sample_t *sample)
{
    uint8_t raw_data[14];

    if (sample == NULL)
    {
        return false;
    }

    if (!mpu6500_read_registers(
            MPU6500_REG_ACCEL_XOUT_H,
            raw_data,
            sizeof(raw_data)))
    {
        return false;
    }

    sample->accel_x =
        mpu6500_decode_signed(raw_data[0], raw_data[1]);
    sample->accel_y =
        mpu6500_decode_signed(raw_data[2], raw_data[3]);
    sample->accel_z =
        mpu6500_decode_signed(raw_data[4], raw_data[5]);
    sample->temperature =
        mpu6500_decode_signed(raw_data[6], raw_data[7]);
    sample->gyro_x =
        mpu6500_decode_signed(raw_data[8], raw_data[9]);
    sample->gyro_y =
        mpu6500_decode_signed(raw_data[10], raw_data[11]);
    sample->gyro_z =
        mpu6500_decode_signed(raw_data[12], raw_data[13]);

    return true;
}
