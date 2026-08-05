#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

/* Onboard status LED: PC13, active-low. */
#define BOARD_STATUS_LED_GPIO_PORT  GPIOC
#define BOARD_STATUS_LED_GPIO_PIN   GPIO_Pin_13
#define BOARD_STATUS_LED_GPIO_CLOCK RCC_APB2Periph_GPIOC
#define BOARD_STATUS_LED_ACTIVE_LOW (1)

/*
 * External push button:
 *
 *   PA0 ----- push button ----- GND
 *
 * PA0 uses the STM32 internal pull-up resistor. A press therefore generates
 * a falling edge on EXTI line 0.
 */
#define BOARD_USER_BUTTON_GPIO_PORT        GPIOA
#define BOARD_USER_BUTTON_GPIO_PIN         GPIO_Pin_0
#define BOARD_USER_BUTTON_GPIO_CLOCK       RCC_APB2Periph_GPIOA
#define BOARD_USER_BUTTON_AFIO_CLOCK       RCC_APB2Periph_AFIO
#define BOARD_USER_BUTTON_PORT_SOURCE      GPIO_PortSourceGPIOA
#define BOARD_USER_BUTTON_PIN_SOURCE       GPIO_PinSource0
#define BOARD_USER_BUTTON_EXTI_LINE        EXTI_Line0
#define BOARD_USER_BUTTON_IRQn             EXTI0_IRQn
#define BOARD_USER_BUTTON_IRQ_PRIORITY     (2U)
#define BOARD_USER_BUTTON_ACTIVE_LOW       (1)

#endif /* BOARD_PINS_H */
