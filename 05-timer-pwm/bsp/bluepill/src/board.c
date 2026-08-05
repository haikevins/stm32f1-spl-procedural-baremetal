#include "board.h"

#include "board_pwm.h"
#include "board_timebase.h"
#include "stm32f10x.h"

bool board_init(void)
{
    SystemCoreClockUpdate();

    if (!board_pwm_init())
    {
        return false;
    }

    if (!board_timebase_init())
    {
        return false;
    }

    return true;
}
