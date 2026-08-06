#include "board.h"

#include "board_display_bus.h"
#include "board_timebase.h"
#include "stm32f10x.h"

bool board_init(void)
{
    SystemCoreClockUpdate();

    /*
     * The timebase starts first because the external display driver uses it
     * for power-on timing and bounded I2C polling.
     */
    if (!board_timebase_init())
    {
        return false;
    }

    if (!board_display_bus_init())
    {
        return false;
    }

    return true;
}
