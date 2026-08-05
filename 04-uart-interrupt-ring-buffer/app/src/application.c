#include "application.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "uart_service.h"

#define APPLICATION_PROCESS_BUDGET (32U)

static const uint8_t s_startup_message[] =
    "\r\n"
    "STM32F103 UART interrupt + ring buffer ready\r\n"
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
    uint32_t budget = APPLICATION_PROCESS_BUDGET;

    /*
     * Fill the TX ring without waiting for individual hardware transfers.
     * TXE interrupts drain the ring in the background.
     */
    while ((budget > 0U) &&
           (s_startup_message_index <
            (sizeof(s_startup_message) - 1U)))
    {
        if (!uart_service_try_write_byte(
                s_startup_message[s_startup_message_index]))
        {
            return;
        }

        ++s_startup_message_index;
        --budget;
    }

    if (s_startup_message_index <
        (sizeof(s_startup_message) - 1U))
    {
        return;
    }

    while (budget > 0U)
    {
        uint8_t received_byte;

        if (s_echo_pending)
        {
            if (!uart_service_try_write_byte(s_echo_byte))
            {
                return;
            }

            s_echo_pending = false;
            --budget;
            continue;
        }

        if (!uart_service_try_read_byte(&received_byte))
        {
            return;
        }

        if (!uart_service_try_write_byte(received_byte))
        {
            /*
             * Preserve the byte when the TX ring is temporarily full.
             * It is retried before another RX byte is consumed.
             */
            s_echo_byte = received_byte;
            s_echo_pending = true;
            return;
        }

        --budget;
    }
}

bool application_has_pending_work(void)
{
    if (s_startup_message_index <
        (sizeof(s_startup_message) - 1U))
    {
        return uart_service_can_write();
    }

    if (s_echo_pending)
    {
        return uart_service_can_write();
    }

    return uart_service_can_read();
}
