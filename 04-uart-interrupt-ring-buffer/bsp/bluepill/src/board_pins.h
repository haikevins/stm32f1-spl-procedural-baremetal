#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

/*
 * USART1 console:
 *
 *   PA9  (USART1_TX) -> USB-to-UART RX
 *   PA10 (USART1_RX) <- USB-to-UART TX
 *   GND              -- common ground
 */
#define BOARD_UART_USART              USART1
#define BOARD_UART_USART_CLOCK        RCC_APB2Periph_USART1
#define BOARD_UART_USART_IRQn         USART1_IRQn
#define BOARD_UART_GPIO_PORT          GPIOA
#define BOARD_UART_GPIO_CLOCK         RCC_APB2Periph_GPIOA
#define BOARD_UART_TX_GPIO_PIN        GPIO_Pin_9
#define BOARD_UART_RX_GPIO_PIN        GPIO_Pin_10

#endif /* BOARD_PINS_H */
