#include "bsp.h"

#include "stm32f10x.h"

void BSP_Init(void)
{
    /* SystemInit() runs before main(); refresh the exported clock value. */
    SystemCoreClockUpdate();

    /* Add board-level peripheral initialization here. */
}
