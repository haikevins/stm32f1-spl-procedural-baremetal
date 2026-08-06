#include "board.h"

#include "board_adc_dma.h"
#include "board_led.h"
#include "stm32f10x.h"

bool board_init(void)
{
    SystemCoreClockUpdate();

    board_led_init();

    if (!board_adc_dma_init())
    {
        return false;
    }

    return true;
}
