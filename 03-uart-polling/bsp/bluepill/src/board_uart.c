#include "board_uart.h"

#include <stddef.h>

#include "board_config.h"
#include "board_pins.h"

void board_uart_init(void)
{
    GPIO_InitTypeDef gpio_init;
    USART_InitTypeDef usart_init;

    RCC_APB2PeriphClockCmd(
        BOARD_UART_GPIO_CLOCK | BOARD_UART_USART_CLOCK,
        ENABLE);

    gpio_init.GPIO_Pin = BOARD_UART_TX_GPIO_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(BOARD_UART_GPIO_PORT, &gpio_init);

    gpio_init.GPIO_Pin = BOARD_UART_RX_GPIO_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(BOARD_UART_GPIO_PORT, &gpio_init);

    USART_StructInit(&usart_init);
    usart_init.USART_BaudRate = BOARD_UART_BAUD_RATE;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;
    usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(BOARD_UART_USART, &usart_init);
    USART_Cmd(BOARD_UART_USART, ENABLE);
}

bool board_uart_try_read_byte(uint8_t *byte)
{
    if (byte == NULL)
    {
        return false;
    }

    if (USART_GetFlagStatus(
            BOARD_UART_USART,
            USART_FLAG_RXNE) == RESET)
    {
        return false;
    }

    *byte = (uint8_t)USART_ReceiveData(BOARD_UART_USART);

    return true;
}

bool board_uart_try_write_byte(uint8_t byte)
{
    if (USART_GetFlagStatus(
            BOARD_UART_USART,
            USART_FLAG_TXE) == RESET)
    {
        return false;
    }

    USART_SendData(BOARD_UART_USART, byte);

    return true;
}
