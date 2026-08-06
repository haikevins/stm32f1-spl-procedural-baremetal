#include "application.h"
#include "system.h"

int main(void)
{
    if (!system_init())
    {
        system_panic();
    }

    for (;;)
    {
        application_process();
        system_idle();
    }
}
