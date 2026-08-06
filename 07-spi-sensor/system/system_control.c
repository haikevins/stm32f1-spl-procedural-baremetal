#include "system.h"

#include "stm32f10x.h"

void system_idle(void)
{
    /*
     * Keep the core in Run mode so ST-Link probes without an NRST
     * connection can attach reliably after the firmware starts.
     */
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
