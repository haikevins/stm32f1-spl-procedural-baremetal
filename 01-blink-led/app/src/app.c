#include "app.h"

#include <stdint.h>

#include "bsp_led.h"
#include "system_time.h"

#define APP_BLINK_PERIOD_MS    (500U)

static uint32_t s_last_toggle_ms;

void App_Init(void)
{
    if (!System_Time_Init())
    {
        /* SysTick configuration failed; stop for debugger inspection. */
        while (1)
        {
        }
    }

    s_last_toggle_ms = System_Time_GetMs();
}

void App_Run(void)
{
    const uint32_t now_ms = System_Time_GetMs();

    if ((uint32_t)(now_ms - s_last_toggle_ms) >= APP_BLINK_PERIOD_MS)
    {
        s_last_toggle_ms = now_ms;
        BSP_LED_Toggle();
    }
}
