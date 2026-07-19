#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

void time_service_init(void);
uint32_t time_service_get_ms(void);
bool time_service_periodic_due(
    uint32_t *reference_ms,
    uint32_t period_ms);

#endif /* TIME_SERVICE_H */
