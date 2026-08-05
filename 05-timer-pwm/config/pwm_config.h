#ifndef PWM_CONFIG_H
#define PWM_CONFIG_H

/*
 * A 10-permille step every 10 ms produces approximately:
 *   1 second from 0% to 100%
 *   1 second from 100% to 0%
 */
#define PWM_BREATH_UPDATE_PERIOD_MS (10UL)
#define PWM_BREATH_STEP_PERMILLE    (10U)

#endif /* PWM_CONFIG_H */
