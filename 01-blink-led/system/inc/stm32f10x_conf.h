#ifndef STM32F10X_CONF_H
#define STM32F10X_CONF_H

/* SPL peripheral headers used by this example. */
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

/* Disable full assert handling for this minimal release build. */
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line);
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define assert_param(expr) ((void)0U)
#endif

#endif /* STM32F10X_CONF_H */
