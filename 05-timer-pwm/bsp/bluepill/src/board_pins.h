#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

/*
 * External PWM LED:
 *
 *   PA0 (TIM2_CH1) ---- resistor ---- LED anode
 *   LED cathode -------------------- GND
 *
 * Use a suitable current-limiting resistor, for example 330 ohms.
 */
#define BOARD_PWM_GPIO_PORT   GPIOA
#define BOARD_PWM_GPIO_PIN    GPIO_Pin_0
#define BOARD_PWM_GPIO_CLOCK  RCC_APB2Periph_GPIOA

#define BOARD_PWM_TIMER       TIM2
#define BOARD_PWM_TIMER_CLOCK RCC_APB1Periph_TIM2

#endif /* BOARD_PINS_H */
