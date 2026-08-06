# 07 - SPI Sensor: GY-91 MPU6500/MPU9250-Compatible

This example reads the accelerometer, gyroscope and temperature registers of
the MPU6500-class IMU on a GY-91 module through SPI1.

The GY-91 also contains a BMP280 pressure sensor, but that device is not used
in this example. Keep `CSB` high so the BMP280 remains deselected.

## Wiring

Use the 3.3 V domain throughout:

| Blue Pill | GY-91 | Purpose |
|---|---|---|
| 3.3V | 3V3 | Module 3.3 V supply |
| GND | GND | Common ground |
| PA5 | SCL | SPI1 SCK |
| PA6 | SDO/SA0 | SPI1 MISO |
| PA7 | SDA | SPI1 MOSI |
| PA4 | NCS | MPU chip select |
| 3.3V | CSB | Keep BMP280 deselected |

Leave `VIN` unconnected when the module is powered from its `3V3` pin. Do not
connect both `VIN` and `3V3` at the same time.

## Behavior

- SPI1 runs in mode 0 at no more than 1 MHz.
- The driver accepts `WHO_AM_I = 0x70` for MPU6500.
- It also accepts `WHO_AM_I = 0x71` because many GY-91 boards contain an
  MPU9250; the 6-axis accel/gyro/temperature path used here is compatible.
- Accelerometer range: +/-2 g.
- Gyroscope range: +/-250 degrees/s.
- Sample polling period: 10 ms (100 Hz).
- PC13 toggles every 500 ms after successful initialization.
- `system_idle()` uses `__NOP()` for stable SWD re-attachment.

No UART is added, so use GDB to inspect the values.

## GDB variables

```gdb
print/x application_sensor_who_am_i
print application_sensor_sequence
print application_sensor_read_errors

print application_accel_x_raw
print application_accel_y_raw
print application_accel_z_raw

print application_accel_x_mg
print application_accel_y_mg
print application_accel_z_mg

print application_gyro_x_mdps
print application_gyro_y_mdps
print application_gyro_z_mdps

print application_temperature_mdeg_c
```

At rest on a flat table, one accelerometer axis should be near `+1000 mg` or
`-1000 mg`, depending on module orientation. Gyroscope values should be near
zero after the sensor settles, with normal offset and noise.

## Architecture

```text
Application
    |
    v
IMU Service
    |
    v
MPU6500 ECUAL Driver
    |
    v
Board Sensor Bus
    |
    v
GPIO / SPI1
```

The BSP owns STM32 pin and SPI configuration. The ECUAL driver owns the
MPU6500 register protocol. The service converts raw values to integer
engineering units. The application schedules sampling and publishes values
for observation.

## Build

```bash
make check-layers
make clean
make
make flash
```

## Notes

The pin printed as `SDD/SA0` on some boards is normally `SDO/SA0`. In SPI mode
it is the sensor's MISO output.

This example intentionally does not read the AK8963 magnetometer or the
BMP280 pressure sensor.

## SPI interface selection

The driver disables the MPU I2C host interface before reset and once again
after reset, because `USER_CTRL.I2C_IF_DIS` is cleared by device reset. This
keeps the shared SDA/SDI and SCL/SCLK pins in the intended SPI mode before
reading `WHO_AM_I`.
