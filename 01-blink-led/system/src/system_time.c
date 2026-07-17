#include "system_time.h"

#include "stm32f10x.h"

static volatile uint32_t s_tick_ms;

bool System_Time_Init(void)
{
    SystemCoreClockUpdate();
    s_tick_ms = 0U;

    if ((SystemCoreClock < 1000U) ||
        (SysTick_Config(SystemCoreClock / 1000U) != 0U))
    {
        return false;
    }

    return true;
}

uint32_t System_Time_GetMs(void)
{
    return s_tick_ms;
}

void System_Time_TickISR(void)
{
    ++s_tick_ms;
}
