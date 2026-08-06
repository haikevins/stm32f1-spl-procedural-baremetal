#ifndef BOARD_SENSOR_BUS_H
#define BOARD_SENSOR_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool board_sensor_bus_init(void);

bool board_sensor_bus_transfer(
    const uint8_t *transmit_data,
    uint8_t *receive_data,
    size_t length);

void board_sensor_bus_select(void);
void board_sensor_bus_deselect(void);
void board_sensor_bus_delay_ms(uint32_t delay_ms);

#endif /* BOARD_SENSOR_BUS_H */
