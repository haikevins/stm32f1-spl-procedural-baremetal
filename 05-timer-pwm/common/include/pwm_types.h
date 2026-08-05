#ifndef PWM_TYPES_H
#define PWM_TYPES_H

#include <stdint.h>

typedef uint16_t pwm_duty_permille_t;

#define PWM_DUTY_PERMILLE_MIN ((pwm_duty_permille_t)0U)
#define PWM_DUTY_PERMILLE_MAX ((pwm_duty_permille_t)1000U)

#endif /* PWM_TYPES_H */
