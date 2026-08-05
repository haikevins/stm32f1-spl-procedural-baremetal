#include "board.h"

#include "board_button.h"
#include "board_led.h"
#include "board_timebase.h"
#include "stm32f10x.h"

bool board_init(void)
{
    SystemCoreClockUpdate();

    board_led_init();

    if (!board_timebase_init())
    {
        return false;
    }

    board_button_init();

    return true;
}
