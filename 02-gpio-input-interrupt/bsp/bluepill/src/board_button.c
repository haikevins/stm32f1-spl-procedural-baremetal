#include "board_button.h"

#include <stdint.h>

#include "board_pins.h"
#include "stm32f10x.h"

static volatile bool s_press_edge_pending;

static uint32_t interrupt_save(void)
{
    uint32_t primask;

    __asm volatile(
        "mrs %0, primask"
        : "=r"(primask)
        :
        : "memory");

    __disable_irq();

    return primask;
}

static void interrupt_restore(uint32_t primask)
{
    if ((primask & 1U) == 0U)
    {
        __enable_irq();
    }
}

void board_button_init(void)
{
    GPIO_InitTypeDef gpio_init;
    EXTI_InitTypeDef exti_init;

    RCC_APB2PeriphClockCmd(
        BOARD_USER_BUTTON_GPIO_CLOCK |
        BOARD_USER_BUTTON_AFIO_CLOCK,
        ENABLE);

    gpio_init.GPIO_Pin = BOARD_USER_BUTTON_GPIO_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(BOARD_USER_BUTTON_GPIO_PORT, &gpio_init);

    GPIO_EXTILineConfig(
        BOARD_USER_BUTTON_PORT_SOURCE,
        BOARD_USER_BUTTON_PIN_SOURCE);

    EXTI_ClearITPendingBit(BOARD_USER_BUTTON_EXTI_LINE);

    exti_init.EXTI_Line = BOARD_USER_BUTTON_EXTI_LINE;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);

    s_press_edge_pending = false;

    NVIC_DisableIRQ(BOARD_USER_BUTTON_IRQn);
    NVIC_ClearPendingIRQ(BOARD_USER_BUTTON_IRQn);
    NVIC_SetPriority(
        BOARD_USER_BUTTON_IRQn,
        BOARD_USER_BUTTON_IRQ_PRIORITY);
    NVIC_EnableIRQ(BOARD_USER_BUTTON_IRQn);
}

bool board_button_is_pressed(void)
{
    const BitAction input_level = GPIO_ReadInputDataBit(
        BOARD_USER_BUTTON_GPIO_PORT,
        BOARD_USER_BUTTON_GPIO_PIN);

#if BOARD_USER_BUTTON_ACTIVE_LOW
    return input_level == Bit_RESET;
#else
    return input_level == Bit_SET;
#endif
}

bool board_button_take_press_edge(void)
{
    const uint32_t previous_primask = interrupt_save();
    const bool pending = s_press_edge_pending;

    s_press_edge_pending = false;
    interrupt_restore(previous_primask);

    return pending;
}

void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(BOARD_USER_BUTTON_EXTI_LINE) != RESET)
    {
        s_press_edge_pending = true;
        EXTI_ClearITPendingBit(BOARD_USER_BUTTON_EXTI_LINE);
    }
}
