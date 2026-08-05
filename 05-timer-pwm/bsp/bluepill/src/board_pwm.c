#include "board_pwm.h"

#include <stdint.h>

#include "board_config.h"
#include "board_pins.h"
#include "stm32f10x.h"

#if BOARD_PWM_TIMER_TICK_HZ == 0
#error "BOARD_PWM_TIMER_TICK_HZ must be greater than zero"
#endif

#if BOARD_PWM_FREQUENCY_HZ == 0
#error "BOARD_PWM_FREQUENCY_HZ must be greater than zero"
#endif

static uint32_t s_pwm_period_counts;

static uint32_t board_pwm_get_timer_clock_hz(void)
{
    RCC_ClocksTypeDef clocks;
    uint32_t timer_clock_hz;

    RCC_GetClocksFreq(&clocks);
    timer_clock_hz = clocks.PCLK1_Frequency;

    /*
     * STM32F1 timers run at twice PCLK when the APB prescaler is not 1.
     * TIM2 is connected to APB1.
     */
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
    {
        timer_clock_hz *= 2U;
    }

    return timer_clock_hz;
}

bool board_pwm_init(void)
{
    GPIO_InitTypeDef gpio_init;
    TIM_TimeBaseInitTypeDef time_base;
    TIM_OCInitTypeDef output_compare;
    uint32_t timer_clock_hz;
    uint32_t prescaler_divider;
    uint32_t period_counts;

    RCC_APB2PeriphClockCmd(BOARD_PWM_GPIO_CLOCK, ENABLE);
    RCC_APB1PeriphClockCmd(BOARD_PWM_TIMER_CLOCK, ENABLE);

    gpio_init.GPIO_Pin = BOARD_PWM_GPIO_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(BOARD_PWM_GPIO_PORT, &gpio_init);

    timer_clock_hz = board_pwm_get_timer_clock_hz();

    if ((timer_clock_hz % BOARD_PWM_TIMER_TICK_HZ) != 0U)
    {
        return false;
    }

    prescaler_divider =
        timer_clock_hz / BOARD_PWM_TIMER_TICK_HZ;

    if ((prescaler_divider == 0U) ||
        (prescaler_divider > 65536U))
    {
        return false;
    }

    if ((BOARD_PWM_TIMER_TICK_HZ %
         BOARD_PWM_FREQUENCY_HZ) != 0U)
    {
        return false;
    }

    period_counts =
        BOARD_PWM_TIMER_TICK_HZ / BOARD_PWM_FREQUENCY_HZ;

    /*
     * CCR1 must also represent period_counts to generate a true 100%
     * duty cycle, so the value must fit in the 16-bit compare register.
     */
    if ((period_counts == 0U) ||
        (period_counts > 65535U))
    {
        return false;
    }

    s_pwm_period_counts = period_counts;

    TIM_TimeBaseStructInit(&time_base);
    time_base.TIM_Prescaler =
        (uint16_t)(prescaler_divider - 1U);
    time_base.TIM_CounterMode = TIM_CounterMode_Up;
    time_base.TIM_Period =
        (uint16_t)(period_counts - 1U);
    time_base.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(BOARD_PWM_TIMER, &time_base);

    TIM_OCStructInit(&output_compare);
    output_compare.TIM_OCMode = TIM_OCMode_PWM1;
    output_compare.TIM_OutputState = TIM_OutputState_Enable;
    output_compare.TIM_Pulse = 0U;
    output_compare.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(BOARD_PWM_TIMER, &output_compare);

    TIM_OC1PreloadConfig(
        BOARD_PWM_TIMER,
        TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(BOARD_PWM_TIMER, ENABLE);

    TIM_SetCompare1(BOARD_PWM_TIMER, 0U);
    TIM_Cmd(BOARD_PWM_TIMER, ENABLE);

    return true;
}

void board_pwm_set_duty_permille(
    pwm_duty_permille_t duty_permille)
{
    uint32_t compare_counts;

    if (duty_permille > PWM_DUTY_PERMILLE_MAX)
    {
        duty_permille = PWM_DUTY_PERMILLE_MAX;
    }

    compare_counts =
        ((uint32_t)duty_permille * s_pwm_period_counts +
         500U) /
        1000U;

    TIM_SetCompare1(
        BOARD_PWM_TIMER,
        (uint16_t)compare_counts);
}
