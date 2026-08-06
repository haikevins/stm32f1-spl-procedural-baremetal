#include "application.h"

#include <stdbool.h>
#include <stdint.h>

#include "application_config.h"
#include "imu_service.h"
#include "indication_service.h"
#include "time_service.h"

#if SENSOR_SAMPLE_PERIOD_MS == 0
#error "SENSOR_SAMPLE_PERIOD_MS must be greater than zero"
#endif

#if SENSOR_HEARTBEAT_PERIOD_MS == 0
#error "SENSOR_HEARTBEAT_PERIOD_MS must be greater than zero"
#endif

/*
 * These symbols are intentionally global and volatile so the live sensor
 * state is easy to inspect from GDB without adding UART to this example.
 */
volatile uint8_t application_sensor_who_am_i;
volatile uint32_t application_sensor_sequence;
volatile uint32_t application_sensor_read_errors;

volatile int16_t application_accel_x_raw;
volatile int16_t application_accel_y_raw;
volatile int16_t application_accel_z_raw;

volatile int16_t application_gyro_x_raw;
volatile int16_t application_gyro_y_raw;
volatile int16_t application_gyro_z_raw;

volatile int16_t application_temperature_raw;

volatile int32_t application_accel_x_mg;
volatile int32_t application_accel_y_mg;
volatile int32_t application_accel_z_mg;

volatile int32_t application_gyro_x_mdps;
volatile int32_t application_gyro_y_mdps;
volatile int32_t application_gyro_z_mdps;

volatile int32_t application_temperature_mdeg_c;

static uint32_t s_last_sample_ms;
static uint32_t s_last_heartbeat_ms;

static void application_publish_sample(
    const imu_sample_t *sample)
{
    application_accel_x_raw = sample->raw.accel_x;
    application_accel_y_raw = sample->raw.accel_y;
    application_accel_z_raw = sample->raw.accel_z;

    application_gyro_x_raw = sample->raw.gyro_x;
    application_gyro_y_raw = sample->raw.gyro_y;
    application_gyro_z_raw = sample->raw.gyro_z;

    application_temperature_raw =
        sample->raw.temperature;

    application_accel_x_mg = sample->accel_x_mg;
    application_accel_y_mg = sample->accel_y_mg;
    application_accel_z_mg = sample->accel_z_mg;

    application_gyro_x_mdps =
        sample->gyro_x_millidegrees_per_second;
    application_gyro_y_mdps =
        sample->gyro_y_millidegrees_per_second;
    application_gyro_z_mdps =
        sample->gyro_z_millidegrees_per_second;

    application_temperature_mdeg_c =
        sample->temperature_millidegrees_c;

    application_sensor_sequence++;
}

bool application_init(void)
{
    application_sensor_who_am_i =
        imu_service_get_who_am_i();
    application_sensor_sequence = 0U;
    application_sensor_read_errors = 0U;

    application_accel_x_raw = 0;
    application_accel_y_raw = 0;
    application_accel_z_raw = 0;

    application_gyro_x_raw = 0;
    application_gyro_y_raw = 0;
    application_gyro_z_raw = 0;

    application_temperature_raw = 0;

    application_accel_x_mg = 0;
    application_accel_y_mg = 0;
    application_accel_z_mg = 0;

    application_gyro_x_mdps = 0;
    application_gyro_y_mdps = 0;
    application_gyro_z_mdps = 0;

    application_temperature_mdeg_c = 0;

    s_last_sample_ms = time_service_get_ms();
    s_last_heartbeat_ms = s_last_sample_ms;

    return true;
}

void application_process(void)
{
    const uint32_t now_ms = time_service_get_ms();

    if ((now_ms - s_last_sample_ms) >=
        SENSOR_SAMPLE_PERIOD_MS)
    {
        imu_sample_t sample;

        s_last_sample_ms += SENSOR_SAMPLE_PERIOD_MS;

        if (imu_service_read(&sample))
        {
            application_publish_sample(&sample);
        }
        else
        {
            application_sensor_read_errors++;
        }
    }

    if ((now_ms - s_last_heartbeat_ms) >=
        SENSOR_HEARTBEAT_PERIOD_MS)
    {
        s_last_heartbeat_ms += SENSOR_HEARTBEAT_PERIOD_MS;
        indication_service_toggle(INDICATION_STATUS);
    }
}
