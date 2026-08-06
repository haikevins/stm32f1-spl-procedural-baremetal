#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 * ADC1 regular channel 0 is sampled from PA0.
 *
 * TIM3 update events trigger one conversion at a fixed rate.
 * DMA1 Channel 1 stores conversions in a circular buffer.
 */
#define BOARD_ADC_REFERENCE_MV                    (3300UL)
#define BOARD_ADC_MAX_RAW_VALUE                   (4095UL)
#define BOARD_ADC_SAMPLE_RATE_HZ                  (1000UL)
#define BOARD_ADC_TRIGGER_TIMER_TICK_HZ           (1000000UL)
#define BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT         (64U)
#define BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT          \
    (BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT / 2U)
#define BOARD_ADC_CALIBRATION_TIMEOUT_ITERATIONS  (1000000UL)

#endif /* BOARD_CONFIG_H */
