#ifndef PWM_SERVICE_H
#define PWM_SERVICE_H

#include "pwm_types.h"

void pwm_service_init(void);
void pwm_service_set_duty_permille(
    pwm_duty_permille_t duty_permille);

#endif /* PWM_SERVICE_H */
