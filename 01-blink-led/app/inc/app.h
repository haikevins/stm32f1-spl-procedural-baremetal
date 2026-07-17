#ifndef APP_H
#define APP_H

#include <stdbool.h>

/**
 * @brief Initialize the Blink LED application.
 *
 * @return true when all required peripherals were initialized successfully.
 */
bool App_Init(void);

/**
 * @brief Execute one non-blocking application iteration.
 */
void App_Run(void);

#endif /* APP_H */
