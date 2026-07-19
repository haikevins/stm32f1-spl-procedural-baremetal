#include "time_service.h"

#include <stddef.h>

#include "board_timebase.h"

void time_service_init(void)
{
    /* The physical timebase is initialized by board_init(). */
}

uint32_t time_service_get_ms(void)
{
    return board_timebase_get_ms();
}

bool time_service_periodic_due(
    uint32_t *reference_ms,
    uint32_t period_ms)
{
    const uint32_t now_ms = time_service_get_ms();

    if ((reference_ms == NULL) || (period_ms == 0U))
    {
        return false;
    }

    if ((uint32_t)(now_ms - *reference_ms) < period_ms)
    {
        return false;
    }

    *reference_ms = now_ms;
    return true;
}
