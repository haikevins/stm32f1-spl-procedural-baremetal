#ifndef STM32F10X_CONF_H
#define STM32F10X_CONF_H

/*
 * Keep this vendor configuration header small.
 * Each BSP or low-level module should include only the SPL headers it uses.
 */
#include "misc.h"

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line);
#define assert_param(expression)     ((expression) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define assert_param(expression) ((void)0U)
#endif

#endif /* STM32F10X_CONF_H */
