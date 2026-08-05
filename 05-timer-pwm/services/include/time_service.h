#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <stdint.h>

void time_service_init(void);
uint32_t time_service_get_ms(void);
uint32_t time_service_elapsed_ms(uint32_t start_time_ms);

#endif /* TIME_SERVICE_H */
