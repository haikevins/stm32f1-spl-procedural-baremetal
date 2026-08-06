#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "misc.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

/*
 * Analog input:
 *
 *   3.3 V ---- potentiometer ---- GND
 *                    |
 *                    +---- PA0 / ADC1_IN0
 */
#define BOARD_ADC_GPIO_PORT       GPIOA
#define BOARD_ADC_GPIO_PIN        GPIO_Pin_0
#define BOARD_ADC_GPIO_CLOCK      RCC_APB2Periph_GPIOA

#define BOARD_ADC_PERIPHERAL      ADC1
#define BOARD_ADC_CHANNEL         ADC_Channel_0
#define BOARD_ADC_CLOCK           RCC_APB2Periph_ADC1

#define BOARD_ADC_DMA_CHANNEL     DMA1_Channel1
#define BOARD_ADC_DMA_CLOCK       RCC_AHBPeriph_DMA1
#define BOARD_ADC_DMA_IRQn        DMA1_Channel1_IRQn

#define BOARD_ADC_TRIGGER_TIMER       TIM3
#define BOARD_ADC_TRIGGER_TIMER_CLOCK RCC_APB1Periph_TIM3

#define BOARD_STATUS_LED_GPIO_PORT  GPIOC
#define BOARD_STATUS_LED_GPIO_PIN   GPIO_Pin_13
#define BOARD_STATUS_LED_GPIO_CLOCK RCC_APB2Periph_GPIOC
#define BOARD_STATUS_LED_ACTIVE_LOW (1)

#endif /* BOARD_PINS_H */
