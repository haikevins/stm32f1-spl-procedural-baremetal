#include "board_uart.h"

#include <stddef.h>

#include "board_config.h"
#include "board_pins.h"
#include "byte_ring_buffer.h"
#include "stm32f10x.h"

_Static_assert(
    BOARD_UART_RX_BUFFER_SIZE >= 2U,
    "UART RX buffer must contain at least two storage slots");

_Static_assert(
    BOARD_UART_TX_BUFFER_SIZE >= 2U,
    "UART TX buffer must contain at least two storage slots");

static uint8_t s_rx_storage[BOARD_UART_RX_BUFFER_SIZE];
static uint8_t s_tx_storage[BOARD_UART_TX_BUFFER_SIZE];

static byte_ring_buffer_t s_rx_ring;
static byte_ring_buffer_t s_tx_ring;

static volatile uint32_t s_rx_overflow_count;
static volatile uint32_t s_rx_error_count;

void board_uart_init(void)
{
    GPIO_InitTypeDef gpio_init;
    USART_InitTypeDef usart_init;

    (void)byte_ring_buffer_init(
        &s_rx_ring,
        s_rx_storage,
        (uint16_t)sizeof(s_rx_storage));

    (void)byte_ring_buffer_init(
        &s_tx_ring,
        s_tx_storage,
        (uint16_t)sizeof(s_tx_storage));

    s_rx_overflow_count = 0U;
    s_rx_error_count = 0U;

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

    NVIC_SetPriority(
        BOARD_UART_USART_IRQn,
        BOARD_UART_IRQ_PRIORITY);

    NVIC_EnableIRQ(BOARD_UART_USART_IRQn);

    /*
     * RXNE drives received bytes into the RX ring. ERR enables noise,
     * framing, and overrun error handling. TXE remains disabled until
     * the first byte is placed in the TX ring.
     */
    USART_ITConfig(
        BOARD_UART_USART,
        USART_IT_RXNE,
        ENABLE);

    USART_ITConfig(
        BOARD_UART_USART,
        USART_IT_ERR,
        ENABLE);

    USART_Cmd(BOARD_UART_USART, ENABLE);
}

bool board_uart_try_read_byte(uint8_t *byte)
{
    if (byte == NULL)
    {
        return false;
    }

    return byte_ring_buffer_pop(&s_rx_ring, byte);
}

bool board_uart_try_write_byte(uint8_t byte)
{
    if (!byte_ring_buffer_push(&s_tx_ring, byte))
    {
        return false;
    }

    /*
     * TXE may already be set. Enabling TXEIE then immediately pends the
     * USART interrupt, which starts or resumes transmission.
     */
    USART_ITConfig(
        BOARD_UART_USART,
        USART_IT_TXE,
        ENABLE);

    return true;
}

bool board_uart_can_read(void)
{
    return !byte_ring_buffer_is_empty(&s_rx_ring);
}

bool board_uart_can_write(void)
{
    return !byte_ring_buffer_is_full(&s_tx_ring);
}

uint32_t board_uart_get_rx_overflow_count(void)
{
    return s_rx_overflow_count;
}

uint32_t board_uart_get_rx_error_count(void)
{
    return s_rx_error_count;
}

void USART1_IRQHandler(void)
{
    const uint32_t status = BOARD_UART_USART->SR;
    const uint32_t receive_errors =
        status &
        (USART_SR_ORE |
         USART_SR_NE |
         USART_SR_FE |
         USART_SR_PE);

    /*
     * Reading SR above and DR below clears RXNE plus the receive error
     * conditions on STM32F1. Keep the ISR short: it only transfers a byte
     * into the RX ring and records counters.
     */
    if (((status & USART_SR_RXNE) != 0U) ||
        (receive_errors != 0U))
    {
        const uint8_t received_byte =
            (uint8_t)BOARD_UART_USART->DR;

        if (receive_errors != 0U)
        {
            ++s_rx_error_count;
        }

        if (((status & USART_SR_RXNE) != 0U) &&
            !byte_ring_buffer_push(
                &s_rx_ring,
                received_byte))
        {
            ++s_rx_overflow_count;
        }
    }

    if (((status & USART_SR_TXE) != 0U) &&
        ((BOARD_UART_USART->CR1 &
          USART_CR1_TXEIE) != 0U))
    {
        uint8_t transmit_byte;

        if (byte_ring_buffer_pop(
                &s_tx_ring,
                &transmit_byte))
        {
            BOARD_UART_USART->DR = transmit_byte;
        }
        else
        {
            /*
             * No queued byte remains. Disable TXEIE to avoid an interrupt
             * storm while TXE stays asserted.
             */
            BOARD_UART_USART->CR1 &=
                (uint16_t)~USART_CR1_TXEIE;
        }
    }
}
