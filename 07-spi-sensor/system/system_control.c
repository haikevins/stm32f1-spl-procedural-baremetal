#include "system.h"

#include "stm32f10x.h"

void system_idle(void)
{
    __NOP();
}

void system_panic(void)
{
    __disable_irq();

    for (;;)
    {
        __NOP();
    }
}
