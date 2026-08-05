#include "application.h"

#include "button_service.h"
#include "indication_service.h"

void application_init(void)
{
    /* Services are initialized by the system composition root. */
}

void application_process(void)
{
    button_service_process();

    if (button_service_take_pressed_event())
    {
        indication_service_toggle(INDICATION_STATUS);
    }
}
