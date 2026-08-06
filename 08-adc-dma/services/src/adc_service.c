#include "adc_service.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board_adc_dma.h"
#include "board_config.h"

static uint16_t
    s_samples[BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT];

static adc_measurement_t s_latest_measurement;
static bool s_measurement_pending;

void adc_service_init(void)
{
    s_latest_measurement.average_raw = 0U;
    s_latest_measurement.minimum_raw = 0U;
    s_latest_measurement.maximum_raw = 0U;
    s_latest_measurement.millivolts = 0U;
    s_latest_measurement.sequence = 0U;
    s_measurement_pending = false;
}

void adc_service_process(void)
{
    uint16_t sample_count;
    uint16_t index;
    uint16_t minimum;
    uint16_t maximum;
    uint16_t average;
    uint32_t sum;

    if (!board_adc_dma_take_sample_block(
            s_samples,
            BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT,
            &sample_count))
    {
        return;
    }

    if (sample_count == 0U)
    {
        return;
    }

    minimum = s_samples[0];
    maximum = s_samples[0];
    sum = 0U;

    for (index = 0U; index < sample_count; ++index)
    {
        const uint16_t sample = s_samples[index];

        sum += sample;

        if (sample < minimum)
        {
            minimum = sample;
        }

        if (sample > maximum)
        {
            maximum = sample;
        }
    }

    average = (uint16_t)(
        (sum + ((uint32_t)sample_count / 2U)) /
        (uint32_t)sample_count);

    s_latest_measurement.average_raw = average;
    s_latest_measurement.minimum_raw = minimum;
    s_latest_measurement.maximum_raw = maximum;
    s_latest_measurement.millivolts = (uint16_t)(
        (((uint32_t)average * BOARD_ADC_REFERENCE_MV) +
         (BOARD_ADC_MAX_RAW_VALUE / 2U)) /
        BOARD_ADC_MAX_RAW_VALUE);
    ++s_latest_measurement.sequence;

    s_measurement_pending = true;
}

bool adc_service_take_measurement(
    adc_measurement_t *measurement)
{
    if ((measurement == NULL) || !s_measurement_pending)
    {
        return false;
    }

    *measurement = s_latest_measurement;
    s_measurement_pending = false;

    return true;
}

uint32_t adc_service_get_dma_overrun_count(void)
{
    return board_adc_dma_get_overrun_count();
}

uint32_t adc_service_get_dma_error_count(void)
{
    return board_adc_dma_get_error_count();
}
