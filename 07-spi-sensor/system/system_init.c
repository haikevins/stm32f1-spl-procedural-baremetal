#include "system.h"

#include "application.h"
#include "board.h"
#include "imu_service.h"
#include "indication_service.h"
#include "time_service.h"

bool system_init(void)
{
    if (!board_init())
    {
        return false;
    }

    time_service_init();
    indication_service_init();

    if (!imu_service_init())
    {
        return false;
    }

    if (!application_init())
    {
        return false;
    }

    return true;
}
