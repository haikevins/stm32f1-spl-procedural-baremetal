#include "app.h"

#include <stdint.h>

#include "bsp.h"
#include "bsp_led.h"
#include "system_time.h"

#define APP_BLINK_PERIOD_MS    (500U)

static uint32_t s_last_toggle_ms;

bool App_Init(void)
{
    BSP_Init();

    if (!System_Time_Init())
    {
        return false;
    }

    s_last_toggle_ms = System_Time_GetMs();
    return true;
}

void App_Run(void)
{
    const uint32_t now_ms = System_Time_GetMs();

    /* Unsigned subtraction keeps working correctly when the tick wraps around. */
    if ((uint32_t)(now_ms - s_last_toggle_ms) >= APP_BLINK_PERIOD_MS)
    {
        s_last_toggle_ms = now_ms;
        BSP_LED_Toggle();
    }
}
