#include "board_led.h"

#include "board_pins.h"

void board_led_init(void)
{
    GPIO_InitTypeDef gpio_init;

    RCC_APB2PeriphClockCmd(BOARD_STATUS_LED_GPIO_CLOCK, ENABLE);

#if BOARD_STATUS_LED_ACTIVE_LOW
    GPIO_SetBits(
        BOARD_STATUS_LED_GPIO_PORT,
        BOARD_STATUS_LED_GPIO_PIN);
#else
    GPIO_ResetBits(
        BOARD_STATUS_LED_GPIO_PORT,
        BOARD_STATUS_LED_GPIO_PIN);
#endif

    gpio_init.GPIO_Pin = BOARD_STATUS_LED_GPIO_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(BOARD_STATUS_LED_GPIO_PORT, &gpio_init);
}

void board_led_set(bool active)
{
#if BOARD_STATUS_LED_ACTIVE_LOW
    if (active)
    {
        GPIO_ResetBits(
            BOARD_STATUS_LED_GPIO_PORT,
            BOARD_STATUS_LED_GPIO_PIN);
    }
    else
    {
        GPIO_SetBits(
            BOARD_STATUS_LED_GPIO_PORT,
            BOARD_STATUS_LED_GPIO_PIN);
    }
#else
    if (active)
    {
        GPIO_SetBits(
            BOARD_STATUS_LED_GPIO_PORT,
            BOARD_STATUS_LED_GPIO_PIN);
    }
    else
    {
        GPIO_ResetBits(
            BOARD_STATUS_LED_GPIO_PORT,
            BOARD_STATUS_LED_GPIO_PIN);
    }
#endif
}

void board_led_toggle(void)
{
    if (GPIO_ReadOutputDataBit(
            BOARD_STATUS_LED_GPIO_PORT,
            BOARD_STATUS_LED_GPIO_PIN) == Bit_SET)
    {
#if BOARD_STATUS_LED_ACTIVE_LOW
        board_led_set(true);
#else
        board_led_set(false);
#endif
    }
    else
    {
#if BOARD_STATUS_LED_ACTIVE_LOW
        board_led_set(false);
#else
        board_led_set(true);
#endif
    }
}
