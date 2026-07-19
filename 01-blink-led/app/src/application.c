#include "application.h"

#include <stdint.h>

#include "application_config.h"
#include "indication_service.h"
#include "time_service.h"

static uint32_t s_last_toggle_ms;

void application_init(void)
{
    s_last_toggle_ms = time_service_get_ms();
}

void application_process(void)
{
    if (time_service_periodic_due(
            &s_last_toggle_ms,
            APPLICATION_BLINK_PERIOD_MS))
    {
        indication_service_toggle(INDICATION_STATUS);
    }
}
