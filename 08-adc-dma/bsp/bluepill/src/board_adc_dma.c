#include "board_adc_dma.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board_config.h"
#include "board_pins.h"
#include "stm32f10x.h"

#if (BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT < 2U)
#error "BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT must be at least two"
#endif

#if ((BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT % 2U) != 0U)
#error "BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT must be even"
#endif

#if (BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT > 65535U)
#error "BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT must fit DMA CNDTR"
#endif

#if (BOARD_ADC_SAMPLE_RATE_HZ == 0U)
#error "BOARD_ADC_SAMPLE_RATE_HZ must be greater than zero"
#endif

#if (BOARD_ADC_TRIGGER_TIMER_TICK_HZ == 0U)
#error "BOARD_ADC_TRIGGER_TIMER_TICK_HZ must be greater than zero"
#endif

static volatile uint16_t
    s_dma_buffer[BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT];

static uint16_t
    s_completed_block[BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT];

static volatile bool s_block_ready;
static volatile uint32_t s_overrun_count;
static volatile uint32_t s_error_count;


static uint32_t board_adc_dma_irq_save(void)
{
    uint32_t primask;

    __ASM volatile (
        "mrs %0, primask\n"
        "cpsid i"
        : "=r" (primask)
        :
        : "memory");

    return primask;
}

static void board_adc_dma_irq_restore(uint32_t primask)
{
    __ASM volatile (
        "msr primask, %0"
        :
        : "r" (primask)
        : "memory");
}

static uint32_t board_adc_dma_get_timer_clock_hz(void)
{
    RCC_ClocksTypeDef clocks;
    uint32_t timer_clock_hz;

    RCC_GetClocksFreq(&clocks);
    timer_clock_hz = clocks.PCLK1_Frequency;

    /*
     * TIM3 is on APB1. STM32F1 timers receive twice PCLK when the APB
     * prescaler is not one.
     */
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
    {
        timer_clock_hz *= 2U;
    }

    return timer_clock_hz;
}

static bool board_adc_dma_calibrate(void)
{
    uint32_t timeout;

    ADC_ResetCalibration(BOARD_ADC_PERIPHERAL);

    timeout = BOARD_ADC_CALIBRATION_TIMEOUT_ITERATIONS;
    while (ADC_GetResetCalibrationStatus(BOARD_ADC_PERIPHERAL) != RESET)
    {
        if (timeout == 0U)
        {
            return false;
        }

        --timeout;
    }

    ADC_StartCalibration(BOARD_ADC_PERIPHERAL);

    timeout = BOARD_ADC_CALIBRATION_TIMEOUT_ITERATIONS;
    while (ADC_GetCalibrationStatus(BOARD_ADC_PERIPHERAL) != RESET)
    {
        if (timeout == 0U)
        {
            return false;
        }

        --timeout;
    }

    return true;
}

static bool board_adc_dma_configure_trigger_timer(void)
{
    TIM_TimeBaseInitTypeDef timer_init;
    uint32_t timer_clock_hz;
    uint32_t prescaler_divider;
    uint32_t period_counts;

    timer_clock_hz = board_adc_dma_get_timer_clock_hz();

    if ((timer_clock_hz % BOARD_ADC_TRIGGER_TIMER_TICK_HZ) != 0U)
    {
        return false;
    }

    prescaler_divider =
        timer_clock_hz / BOARD_ADC_TRIGGER_TIMER_TICK_HZ;

    if ((prescaler_divider == 0U) ||
        (prescaler_divider > 65536U))
    {
        return false;
    }

    if ((BOARD_ADC_TRIGGER_TIMER_TICK_HZ %
         BOARD_ADC_SAMPLE_RATE_HZ) != 0U)
    {
        return false;
    }

    period_counts =
        BOARD_ADC_TRIGGER_TIMER_TICK_HZ /
        BOARD_ADC_SAMPLE_RATE_HZ;

    if ((period_counts == 0U) ||
        (period_counts > 65536U))
    {
        return false;
    }

    TIM_TimeBaseStructInit(&timer_init);
    timer_init.TIM_Prescaler =
        (uint16_t)(prescaler_divider - 1U);
    timer_init.TIM_CounterMode = TIM_CounterMode_Up;
    timer_init.TIM_Period = (uint16_t)(period_counts - 1U);
    timer_init.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(BOARD_ADC_TRIGGER_TIMER, &timer_init);

    TIM_SelectOutputTrigger(
        BOARD_ADC_TRIGGER_TIMER,
        TIM_TRGOSource_Update);

    return true;
}

static void board_adc_dma_publish_block(uint16_t offset)
{
    uint16_t index;

    if (s_block_ready)
    {
        ++s_overrun_count;
    }

    for (index = 0U;
         index < BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT;
         ++index)
    {
        s_completed_block[index] =
            s_dma_buffer[offset + index];
    }

    s_block_ready = true;
}

bool board_adc_dma_init(void)
{
    GPIO_InitTypeDef gpio_init;
    DMA_InitTypeDef dma_init;
    ADC_InitTypeDef adc_init;
    NVIC_InitTypeDef nvic_init;

    s_block_ready = false;
    s_overrun_count = 0U;
    s_error_count = 0U;

    RCC_AHBPeriphClockCmd(BOARD_ADC_DMA_CLOCK, ENABLE);
    RCC_APB2PeriphClockCmd(
        BOARD_ADC_GPIO_CLOCK | BOARD_ADC_CLOCK,
        ENABLE);
    RCC_APB1PeriphClockCmd(
        BOARD_ADC_TRIGGER_TIMER_CLOCK,
        ENABLE);

    /*
     * PCLK2 is 72 MHz in the default system clock configuration.
     * Division by six gives a 12 MHz ADC clock, within the STM32F1 limit.
     */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    gpio_init.GPIO_Pin = BOARD_ADC_GPIO_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(BOARD_ADC_GPIO_PORT, &gpio_init);

    DMA_DeInit(BOARD_ADC_DMA_CHANNEL);

    DMA_StructInit(&dma_init);
    dma_init.DMA_PeripheralBaseAddr =
        (uint32_t)&BOARD_ADC_PERIPHERAL->DR;
    dma_init.DMA_MemoryBaseAddr =
        (uint32_t)&s_dma_buffer[0];
    dma_init.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma_init.DMA_BufferSize =
        BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT;
    dma_init.DMA_PeripheralInc =
        DMA_PeripheralInc_Disable;
    dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init.DMA_PeripheralDataSize =
        DMA_PeripheralDataSize_HalfWord;
    dma_init.DMA_MemoryDataSize =
        DMA_MemoryDataSize_HalfWord;
    dma_init.DMA_Mode = DMA_Mode_Circular;
    dma_init.DMA_Priority = DMA_Priority_High;
    dma_init.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(BOARD_ADC_DMA_CHANNEL, &dma_init);

    DMA_ClearITPendingBit(DMA1_IT_GL1);
    DMA_ITConfig(
        BOARD_ADC_DMA_CHANNEL,
        DMA_IT_HT | DMA_IT_TC | DMA_IT_TE,
        ENABLE);

    nvic_init.NVIC_IRQChannel = BOARD_ADC_DMA_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1U;
    nvic_init.NVIC_IRQChannelSubPriority = 0U;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    ADC_StructInit(&adc_init);
    adc_init.ADC_Mode = ADC_Mode_Independent;
    adc_init.ADC_ScanConvMode = DISABLE;
    adc_init.ADC_ContinuousConvMode = DISABLE;
    adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
    adc_init.ADC_DataAlign = ADC_DataAlign_Right;
    adc_init.ADC_NbrOfChannel = 1U;
    ADC_Init(BOARD_ADC_PERIPHERAL, &adc_init);

    ADC_RegularChannelConfig(
        BOARD_ADC_PERIPHERAL,
        BOARD_ADC_CHANNEL,
        1U,
        ADC_SampleTime_55Cycles5);

    if (!board_adc_dma_configure_trigger_timer())
    {
        return false;
    }

    ADC_DMACmd(BOARD_ADC_PERIPHERAL, ENABLE);
    DMA_Cmd(BOARD_ADC_DMA_CHANNEL, ENABLE);
    ADC_Cmd(BOARD_ADC_PERIPHERAL, ENABLE);

    if (!board_adc_dma_calibrate())
    {
        return false;
    }

    ADC_ExternalTrigConvCmd(BOARD_ADC_PERIPHERAL, ENABLE);
    TIM_Cmd(BOARD_ADC_TRIGGER_TIMER, ENABLE);

    return true;
}

bool board_adc_dma_take_sample_block(
    uint16_t *samples,
    uint16_t capacity,
    uint16_t *sample_count)
{
    uint32_t primask;
    uint16_t index;

    if ((samples == NULL) ||
        (sample_count == NULL) ||
        (capacity < BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT))
    {
        return false;
    }

    primask = board_adc_dma_irq_save();

    if (!s_block_ready)
    {
        board_adc_dma_irq_restore(primask);
        return false;
    }

    for (index = 0U;
         index < BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT;
         ++index)
    {
        samples[index] = s_completed_block[index];
    }

    s_block_ready = false;
    *sample_count = BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT;

    board_adc_dma_irq_restore(primask);

    return true;
}

uint32_t board_adc_dma_get_overrun_count(void)
{
    return s_overrun_count;
}

uint32_t board_adc_dma_get_error_count(void)
{
    return s_error_count;
}

void DMA1_Channel1_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TE1) != RESET)
    {
        ++s_error_count;
        DMA_ClearITPendingBit(DMA1_IT_TE1);
    }

    if (DMA_GetITStatus(DMA1_IT_HT1) != RESET)
    {
        board_adc_dma_publish_block(0U);
        DMA_ClearITPendingBit(DMA1_IT_HT1);
    }

    if (DMA_GetITStatus(DMA1_IT_TC1) != RESET)
    {
        board_adc_dma_publish_block(
            BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT);
        DMA_ClearITPendingBit(DMA1_IT_TC1);
    }
}
