#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define BOARD_STATUS_LED_GPIO_PORT  GPIOC
#define BOARD_STATUS_LED_GPIO_PIN   GPIO_Pin_13
#define BOARD_STATUS_LED_GPIO_CLOCK RCC_APB2Periph_GPIOC
#define BOARD_STATUS_LED_ACTIVE_LOW (1)

#endif /* BOARD_PINS_H */
