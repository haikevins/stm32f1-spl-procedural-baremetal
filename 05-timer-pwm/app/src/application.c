#include "application.h"

#include <stdbool.h>
#include <stdint.h>

#include "pwm_config.h"
#include "pwm_service.h"
#include "pwm_types.h"
#include "time_service.h"

#if PWM_BREATH_UPDATE_PERIOD_MS == 0
#error "PWM_BREATH_UPDATE_PERIOD_MS must be greater than zero"
#endif

#if (PWM_BREATH_STEP_PERMILLE == 0) || \
    (PWM_BREATH_STEP_PERMILLE > 1000)
#error "PWM_BREATH_STEP_PERMILLE must be in the range 1..1000"
#endif

static uint32_t s_last_update_ms;
static pwm_duty_permille_t s_duty_permille;
static bool s_increasing;

void application_init(void)
{
    s_last_update_ms = time_service_get_ms();
    s_duty_permille = PWM_DUTY_PERMILLE_MIN;
    s_increasing = true;

    pwm_service_set_duty_permille(s_duty_permille);
}

void application_process(void)
{
    if (time_service_elapsed_ms(s_last_update_ms) <
        PWM_BREATH_UPDATE_PERIOD_MS)
    {
        return;
    }

    /*
     * Advance by one fixed period instead of assigning the current time.
     * This avoids accumulating small scheduling errors in the waveform.
     */
    s_last_update_ms += PWM_BREATH_UPDATE_PERIOD_MS;

    if (s_increasing)
    {
        if (s_duty_permille >=
            (PWM_DUTY_PERMILLE_MAX -
             (pwm_duty_permille_t)PWM_BREATH_STEP_PERMILLE))
        {
            s_duty_permille = PWM_DUTY_PERMILLE_MAX;
            s_increasing = false;
        }
        else
        {
            s_duty_permille +=
                (pwm_duty_permille_t)PWM_BREATH_STEP_PERMILLE;
        }
    }
    else
    {
        if (s_duty_permille <=
            (pwm_duty_permille_t)PWM_BREATH_STEP_PERMILLE)
        {
            s_duty_permille = PWM_DUTY_PERMILLE_MIN;
            s_increasing = true;
        }
        else
        {
            s_duty_permille -=
                (pwm_duty_permille_t)PWM_BREATH_STEP_PERMILLE;
        }
    }

    pwm_service_set_duty_permille(s_duty_permille);
}
