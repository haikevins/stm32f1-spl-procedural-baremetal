#include "bsp_led.h"

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define BSP_LED_GPIO_PORT       GPIOC
#define BSP_LED_GPIO_PIN        GPIO_Pin_13
#define BSP_LED_GPIO_CLOCK      RCC_APB2Periph_GPIOC

void BSP_LED_Init(void)
{
    GPIO_InitTypeDef gpio_init;

    RCC_APB2PeriphClockCmd(BSP_LED_GPIO_CLOCK, ENABLE);

    /* Drive PC13 high before enabling output mode because the LED is active-low. */
    GPIO_SetBits(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN);

    gpio_init.GPIO_Pin = BSP_LED_GPIO_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(BSP_LED_GPIO_PORT, &gpio_init);
}

void BSP_LED_On(void)
{
    GPIO_ResetBits(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN);
}

void BSP_LED_Off(void)
{
    GPIO_SetBits(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN);
}

void BSP_LED_Toggle(void)
{
    if (GPIO_ReadOutputDataBit(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN) != Bit_RESET)
    {
        BSP_LED_On();
    }
    else
    {
        BSP_LED_Off();
    }
}
