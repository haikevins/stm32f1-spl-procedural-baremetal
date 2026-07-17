#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Configure SysTick to generate a 1 ms interrupt.
 */
bool System_Time_Init(void);

/**
 * @brief Return milliseconds elapsed since System_Time_Init().
 */
uint32_t System_Time_GetMs(void);

/**
 * @brief Increment the millisecond tick from SysTick_Handler().
 *
 * This function is intended for interrupt context only.
 */
void System_Time_TickISR(void);

#endif /* SYSTEM_TIME_H */
