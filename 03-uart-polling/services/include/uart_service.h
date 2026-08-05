#ifndef UART_SERVICE_H
#define UART_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

void uart_service_init(void);
bool uart_service_try_read_byte(uint8_t *byte);
bool uart_service_try_write_byte(uint8_t byte);

#endif /* UART_SERVICE_H */
