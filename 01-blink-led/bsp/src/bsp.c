#include "bsp.h"

#include "bsp_led.h"
#include "stm32f10x.h"

void BSP_Init(void)
{
    /* Keep the template baseline, then add resources required by this example. */
    SystemCoreClockUpdate();
    BSP_LED_Init();
}
