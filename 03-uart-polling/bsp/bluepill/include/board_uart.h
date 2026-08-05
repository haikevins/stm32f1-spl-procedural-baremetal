#ifndef BOARD_UART_H
#define BOARD_UART_H

#include <stdbool.h>
#include <stdint.h>

void board_uart_init(void);
bool board_uart_try_read_byte(uint8_t *byte);
bool board_uart_try_write_byte(uint8_t byte);

#endif /* BOARD_UART_H */
