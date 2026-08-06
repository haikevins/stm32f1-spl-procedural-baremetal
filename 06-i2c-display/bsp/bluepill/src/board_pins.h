#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"

/*
 * SSD1306 128x64 OLED module configured for two-wire I2C:
 *
 *   PB6  I2C1_SCL -> SCL
 *   PB7  I2C1_SDA -> SDA
 *
 * Most four-pin SSD1306 modules already contain pull-up resistors.
 * The bus still requires pull-ups to 3.3 V if the module does not have them.
 */
#define BOARD_DISPLAY_I2C                  I2C1
#define BOARD_DISPLAY_I2C_CLOCK            RCC_APB1Periph_I2C1

#define BOARD_DISPLAY_GPIO_CLOCK           RCC_APB2Periph_GPIOB
#define BOARD_DISPLAY_GPIO_PORT            GPIOB
#define BOARD_DISPLAY_SCL_PIN              GPIO_Pin_6
#define BOARD_DISPLAY_SDA_PIN              GPIO_Pin_7

#endif /* BOARD_PINS_H */
