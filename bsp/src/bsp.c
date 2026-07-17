#include "bsp.h"

#include "stm32f10x.h"
#include "system_stm32f10x.h"

void BSP_Init(void)
{
    /*
     * SystemInit() is called by the startup code before main().
     * Refresh the exported clock value, then add board-specific setup here.
     */
    SystemCoreClockUpdate();
}
