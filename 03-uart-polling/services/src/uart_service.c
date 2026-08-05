#include "uart_service.h"

#include <stddef.h>

#include "board_uart.h"

void uart_service_init(void)
{
    /* The physical USART peripheral is initialized by board_init(). */
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
