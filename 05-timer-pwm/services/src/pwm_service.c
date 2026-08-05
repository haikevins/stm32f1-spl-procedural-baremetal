#include "pwm_service.h"

#include "board_pwm.h"

void pwm_service_init(void)
{
    board_pwm_set_duty_permille(PWM_DUTY_PERMILLE_MIN);
}

void pwm_service_set_duty_permille(
    pwm_duty_permille_t duty_permille)
{
    board_pwm_set_duty_permille(duty_permille);
}
