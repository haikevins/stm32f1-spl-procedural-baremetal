#include "system.h"

#include "application.h"
#include "board.h"
#include "uart_service.h"

bool system_init(void)
{
    if (!board_init())
    {
        return false;
    }

    uart_service_init();
    application_init();

    return true;
}
