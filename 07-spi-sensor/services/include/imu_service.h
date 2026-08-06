#ifndef IMU_SERVICE_H
#define IMU_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "imu_types.h"

bool imu_service_init(void);
bool imu_service_read(imu_sample_t *sample);

imu_device_t imu_service_get_device(void);
uint8_t imu_service_get_who_am_i(void);

#endif /* IMU_SERVICE_H */
