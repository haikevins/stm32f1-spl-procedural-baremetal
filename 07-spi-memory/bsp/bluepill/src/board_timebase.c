#include "board_timebase.h"

#include "board_config.h"
#include "stm32f10x.h"

#if BOARD_TIMEBASE_HZ == 0
#error "BOARD_TIMEBASE_HZ must be greater than zero"
#endif

static volatile uint32_t s_time_ms;

bool board_timebase_init(void)
{
    SystemCoreClockUpdate();
    s_time_ms = 0U;

    if (SysTick_Config(SystemCoreClock / BOARD_TIMEBASE_HZ) != 0U)
    {
        return false;
    }

    NVIC_SetPriority(
        SysTick_IRQn,
        (1UL << __NVIC_PRIO_BITS) - 1UL);

    return true;
}

uint32_t board_timebase_get_ms(void)
{
    return s_time_ms;
}

void SysTick_Handler(void)
{
    s_time_ms++;
}
