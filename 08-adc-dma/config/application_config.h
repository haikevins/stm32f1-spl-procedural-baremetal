#ifndef APPLICATION_CONFIG_H
#define APPLICATION_CONFIG_H

/*
 * The onboard PC13 LED is active when the filtered ADC voltage is above the
 * upper threshold and turns off below the lower threshold.
 *
 * The gap provides hysteresis and prevents flicker around one exact voltage.
 */
#define APPLICATION_ADC_LED_ON_THRESHOLD_MV  (1800U)
#define APPLICATION_ADC_LED_OFF_THRESHOLD_MV (1500U)

#endif /* APPLICATION_CONFIG_H */
