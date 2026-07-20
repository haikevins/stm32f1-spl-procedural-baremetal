#include "system.h"

#include "stm32f10x.h"

void system_idle(void)
{
    __WFI();
}

void system_panic(void)
{
    __disable_irq();

    for (;;)
    {
        __WFI();
    }
}
