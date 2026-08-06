#ifndef MPU6500_H
#define MPU6500_H

#include <stdbool.h>
#include <stdint.h>

#include "imu_types.h"

#define MPU6500_WHO_AM_I_VALUE (0x70U)
#define MPU9250_WHO_AM_I_VALUE (0x71U)

bool mpu6500_init(uint8_t *who_am_i);
bool mpu6500_read_raw(imu_raw_sample_t *sample);

#endif /* MPU6500_H */
