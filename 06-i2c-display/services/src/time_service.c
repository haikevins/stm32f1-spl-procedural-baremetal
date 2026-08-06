#include "time_service.h"

#include "board_timebase.h"

void time_service_init(void)
{
    /* The physical timebase is initialized by board_init(). */
}

uint32_t time_service_get_ms(void)
{
    return board_timebase_get_ms();
}

uint32_t time_service_elapsed_ms(uint32_t start_time_ms)
{
    return time_service_get_ms() - start_time_ms;
}
