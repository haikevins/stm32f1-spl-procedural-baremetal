#include "application.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "uart_service.h"

static const uint8_t s_startup_message[] =
    "\r\n"
    "STM32F103 UART polling ready\r\n"
    "Type characters to echo.\r\n";

static size_t s_startup_message_index;
static bool s_echo_pending;
static uint8_t s_echo_byte;

void application_init(void)
{
    s_startup_message_index = 0U;
    s_echo_pending = false;
    s_echo_byte = 0U;
}

void application_process(void)
{
    /*
     * Transmit the startup message one byte per successful TXE poll.
     * Returning immediately keeps the application loop cooperative.
     */
    if (s_startup_message_index < (sizeof(s_startup_message) - 1U))
    {
        if (uart_service_try_write_byte(
                s_startup_message[s_startup_message_index]))
        {
            ++s_startup_message_index;
        }

        return;
    }

    /*
     * Keep at most one byte pending between RX and TX. No interrupt, DMA,
     * delay loop, or unbounded wait is used.
     */
    if (!s_echo_pending)
    {
        uint8_t received_byte;

        if (uart_service_try_read_byte(&received_byte))
        {
            s_echo_byte = received_byte;
            s_echo_pending = true;
        }
    }

    if (s_echo_pending && uart_service_try_write_byte(s_echo_byte))
    {
        s_echo_pending = false;
    }
}
