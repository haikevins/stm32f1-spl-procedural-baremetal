#include "board.h"

#include "board_config.h"
#include "board_led.h"
#include "board_memory_bus.h"
#include "board_timebase.h"
#include "stm32f10x.h"

bool board_init(void)
{
    SystemCoreClockUpdate();

    if (!board_timebase_init())
    {
        return false;
    }

    board_led_init();

    if (!board_memory_bus_init())
    {
        return false;
    }

    board_memory_bus_delay_ms(
        BOARD_MEMORY_POWER_ON_DELAY_MS);

    return true;
}
