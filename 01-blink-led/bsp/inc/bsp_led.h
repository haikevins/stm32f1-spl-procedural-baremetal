#ifndef BSP_LED_H
#define BSP_LED_H

/**
 * @brief Initialize the onboard LED GPIO.
 *
 * Target board: common STM32F103C8T6 Blue Pill.
 * The onboard LED is connected to PC13 and is active-low.
 */
void BSP_LED_Init(void);

void BSP_LED_On(void);
void BSP_LED_Off(void);
void BSP_LED_Toggle(void);

#endif /* BSP_LED_H */
