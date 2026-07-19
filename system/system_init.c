#include "system.h"

#include "application.h"
#include "board.h"

bool system_init(void)
{
    if (!board_init())
    {
        return false;
    }

    application_init();
    return true;
}
