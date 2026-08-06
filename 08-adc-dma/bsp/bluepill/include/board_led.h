#ifndef BOARD_LED_H
#define BOARD_LED_H

#include <stdbool.h>

void board_led_init(void);
void board_led_set(bool active);
void board_led_toggle(void);

#endif /* BOARD_LED_H */
