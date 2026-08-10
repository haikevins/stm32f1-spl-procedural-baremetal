#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

/*
 * W25Q64 six-pin SPI module:
 *
 *   PA5  SPI1_SCK  -> CLK
 *   PA6  SPI1_MISO <- D1 / DO / IO1
 *   PA7  SPI1_MOSI -> D0 / DI / IO0
 *   PA4            -> CS
 *
 * Power:
 *   Blue Pill 3.3V -> VCC
 *   Blue Pill GND  -> GND
 */
#define BOARD_MEMORY_SPI                    SPI1
#define BOARD_MEMORY_SPI_CLOCK              RCC_APB2Periph_SPI1
#define BOARD_MEMORY_GPIO_CLOCK             RCC_APB2Periph_GPIOA

#define BOARD_MEMORY_SCK_PORT               GPIOA
#define BOARD_MEMORY_SCK_PIN                GPIO_Pin_5

#define BOARD_MEMORY_MISO_PORT              GPIOA
#define BOARD_MEMORY_MISO_PIN               GPIO_Pin_6

#define BOARD_MEMORY_MOSI_PORT              GPIOA
#define BOARD_MEMORY_MOSI_PIN               GPIO_Pin_7

#define BOARD_MEMORY_CS_PORT                GPIOA
#define BOARD_MEMORY_CS_PIN                 GPIO_Pin_4

#define BOARD_STATUS_LED_GPIO_PORT           GPIOC
#define BOARD_STATUS_LED_GPIO_PIN            GPIO_Pin_13
#define BOARD_STATUS_LED_GPIO_CLOCK          RCC_APB2Periph_GPIOC
#define BOARD_STATUS_LED_ACTIVE_LOW          (1)

#endif /* BOARD_PINS_H */
