#ifndef BOARD_UART_H
#define BOARD_UART_H

#include <stdbool.h>
#include <stdint.h>

void board_uart_init(void);

bool board_uart_try_read_byte(uint8_t *byte);
bool board_uart_try_write_byte(uint8_t byte);

bool board_uart_can_read(void);
bool board_uart_can_write(void);

uint32_t board_uart_get_rx_overflow_count(void);
uint32_t board_uart_get_rx_error_count(void);

#endif /* BOARD_UART_H */
