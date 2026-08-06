#include "board.h"

#include "board_config.h"
#include "board_led.h"
#include "board_sensor_bus.h"
#include "board_timebase.h"
#include "stm32f10x.h"

bool board_init(void)
{
    SystemCoreClockUpdate();

    /*
     * Start the timebase first because the sensor driver uses it for
     * power-up and reset delays, and for bounded SPI polling.
     */
    if (!board_timebase_init())
    {
        return false;
    }

    board_led_init();

    if (!board_sensor_bus_init())
    {
        return false;
    }

    board_sensor_bus_delay_ms(
        BOARD_SENSOR_POWER_ON_DELAY_MS);

    return true;
}
