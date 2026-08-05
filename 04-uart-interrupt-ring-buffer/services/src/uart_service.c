#include "uart_service.h"

#include <stddef.h>

#include "board_uart.h"

void uart_service_init(void)
{
    /* The physical USART and ring buffers are initialized by board_init(). */
}

bool uart_service_try_read_byte(uint8_t *byte)
{
    if (byte == NULL)
    {
        return false;
    }

    return board_uart_try_read_byte(byte);
}

bool uart_service_try_write_byte(uint8_t byte)
{
    return board_uart_try_write_byte(byte);
}

bool uart_service_can_read(void)
{
    return board_uart_can_read();
}

bool uart_service_can_write(void)
{
    return board_uart_can_write();
}

uint32_t uart_service_get_rx_overflow_count(void)
{
    return board_uart_get_rx_overflow_count();
}

uint32_t uart_service_get_rx_error_count(void)
{
    return board_uart_get_rx_error_count();
}
