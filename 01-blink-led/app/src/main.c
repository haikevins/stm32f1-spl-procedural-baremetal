#include "app.h"
#include "bsp.h"

int main(void)
{
    BSP_Init();
    App_Init();

    while (1)
    {
        App_Run();
    }
}
