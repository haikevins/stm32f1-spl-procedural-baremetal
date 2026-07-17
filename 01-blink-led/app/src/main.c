#include "app.h"

int main(void)
{
    if (!App_Init())
    {
        /* Initialization failed. Stay here so the debugger can inspect state. */
        while (1)
        {
            /* Intentionally empty. */
        }
    }

    while (1)
    {
        App_Run();
    }
}
