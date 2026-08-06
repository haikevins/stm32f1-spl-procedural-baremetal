#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

/*
 * GY-91 MPU6500/MPU9250-compatible SPI connection:
 *
 *   PA5  SPI1_SCK  -> SCL
 *   PA6  SPI1_MISO <- SDO / SA0
 *   PA7  SPI1_MOSI -> SDA
 *   PA4            -> NCS
 *
 * Power:
 *   Blue Pill 3.3V -> GY-91 3V3
 *   Blue Pill GND  -> GY-91 GND
 *
 * Leave VIN unconnected when powering through 3V3.
 * Connect CSB to 3.3V to keep the BMP280 on the module deselected.
 */
#define BOARD_SENSOR_SPI                   SPI1
#define BOARD_SENSOR_SPI_CLOCK             RCC_APB2Periph_SPI1
#define BOARD_SENSOR_GPIO_CLOCK            RCC_APB2Periph_GPIOA

#define BOARD_SENSOR_SCK_PORT              GPIOA
#define BOARD_SENSOR_SCK_PIN               GPIO_Pin_5

#define BOARD_SENSOR_MISO_PORT             GPIOA
#define BOARD_SENSOR_MISO_PIN              GPIO_Pin_6

#define BOARD_SENSOR_MOSI_PORT             GPIOA
#define BOARD_SENSOR_MOSI_PIN              GPIO_Pin_7

#define BOARD_SENSOR_NCS_PORT              GPIOA
#define BOARD_SENSOR_NCS_PIN               GPIO_Pin_4

#define BOARD_STATUS_LED_GPIO_PORT          GPIOC
#define BOARD_STATUS_LED_GPIO_PIN           GPIO_Pin_13
#define BOARD_STATUS_LED_GPIO_CLOCK         RCC_APB2Periph_GPIOC
#define BOARD_STATUS_LED_ACTIVE_LOW         (1)

#endif /* BOARD_PINS_H */
