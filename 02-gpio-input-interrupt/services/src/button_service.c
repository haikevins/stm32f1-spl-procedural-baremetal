#include "button_service.h"

#include <stdint.h>

#include "board_button.h"
#include "button_config.h"
#include "time_service.h"

static bool s_debounce_active;
static bool s_pressed_event_pending;
static uint32_t s_debounce_started_ms;

void button_service_init(void)
{
    s_debounce_active = false;
    s_pressed_event_pending = false;
    s_debounce_started_ms = time_service_get_ms();

    /* Discard an edge that may have occurred during board initialization. */
    (void)board_button_take_press_edge();
}

void button_service_process(void)
{
    if (board_button_take_press_edge())
    {
        /*
         * Every new falling edge restarts the debounce window. This absorbs
         * switch bounce without delaying inside the ISR or the super-loop.
         */
        s_debounce_started_ms = time_service_get_ms();
        s_debounce_active = true;
    }

    if (!s_debounce_active)
    {
        return;
    }

    if (time_service_elapsed_ms(s_debounce_started_ms) <
        BUTTON_DEBOUNCE_TIME_MS)
    {
        return;
    }

    s_debounce_active = false;

    if (board_button_is_pressed())
    {
        s_pressed_event_pending = true;
    }
}

bool button_service_take_pressed_event(void)
{
    const bool pending = s_pressed_event_pending;
    s_pressed_event_pending = false;

    return pending;
}
