#ifndef BOARD_PWM_H
#define BOARD_PWM_H

#include <stdbool.h>

#include "pwm_types.h"

bool board_pwm_init(void);
void board_pwm_set_duty_permille(
    pwm_duty_permille_t duty_permille);

#endif /* BOARD_PWM_H */
