#include "board.h"

#include "board_uart.h"
#include "stm32f10x.h"

bool board_init(void)
{
    SystemCoreClockUpdate();
    board_uart_init();

    return true;
}
