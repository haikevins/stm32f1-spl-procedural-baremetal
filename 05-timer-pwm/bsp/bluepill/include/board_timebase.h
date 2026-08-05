#ifndef BOARD_TIMEBASE_H
#define BOARD_TIMEBASE_H

#include <stdbool.h>
#include <stdint.h>

bool board_timebase_init(void);
uint32_t board_timebase_get_ms(void);

#endif /* BOARD_TIMEBASE_H */
