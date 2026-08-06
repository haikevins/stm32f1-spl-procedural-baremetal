#include "board_sensor_bus.h"

#include "board_config.h"
#include "board_pins.h"
#include "board_timebase.h"
#include "stm32f10x.h"

#if BOARD_SENSOR_SPI_MAX_HZ == 0
#error "BOARD_SENSOR_SPI_MAX_HZ must be greater than zero"
#endif

#if BOARD_SENSOR_SPI_TIMEOUT_MS == 0
#error "BOARD_SENSOR_SPI_TIMEOUT_MS must be greater than zero"
#endif

static uint16_t board_sensor_select_prescaler(
    uint32_t peripheral_clock_hz)
{
    if (((peripheral_clock_hz + 1U) / 2U) <=
        BOARD_SENSOR_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_2;
    }

    if (((peripheral_clock_hz + 3U) / 4U) <=
        BOARD_SENSOR_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_4;
    }

    if (((peripheral_clock_hz + 7U) / 8U) <=
        BOARD_SENSOR_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_8;
    }

    if (((peripheral_clock_hz + 15U) / 16U) <=
        BOARD_SENSOR_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_16;
    }

    if (((peripheral_clock_hz + 31U) / 32U) <=
        BOARD_SENSOR_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_32;
    }

    if (((peripheral_clock_hz + 63U) / 64U) <=
        BOARD_SENSOR_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_64;
    }

    if (((peripheral_clock_hz + 127U) / 128U) <=
        BOARD_SENSOR_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_128;
    }

    if (((peripheral_clock_hz + 255U) / 256U) <=
        BOARD_SENSOR_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_256;
    }

    return 0U;
}

static bool board_sensor_wait_for_flag(
    uint16_t flag,
    FlagStatus expected_status)
{
    const uint32_t start_ms = board_timebase_get_ms();

    while (SPI_I2S_GetFlagStatus(
               BOARD_SENSOR_SPI,
               flag) != expected_status)
    {
        if ((board_timebase_get_ms() - start_ms) >=
            BOARD_SENSOR_SPI_TIMEOUT_MS)
        {
            return false;
        }
    }

    return true;
}

bool board_sensor_bus_init(void)
{
    GPIO_InitTypeDef gpio_init;
    SPI_InitTypeDef spi_init;
    RCC_ClocksTypeDef clocks;
    uint16_t spi_prescaler;

    RCC_APB2PeriphClockCmd(
        BOARD_SENSOR_GPIO_CLOCK |
        BOARD_SENSOR_SPI_CLOCK,
        ENABLE);

    board_sensor_bus_deselect();

    gpio_init.GPIO_Pin =
        BOARD_SENSOR_SCK_PIN |
        BOARD_SENSOR_MOSI_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio_init);

    gpio_init.GPIO_Pin = BOARD_SENSOR_MISO_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(BOARD_SENSOR_MISO_PORT, &gpio_init);

    gpio_init.GPIO_Pin = BOARD_SENSOR_NCS_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(BOARD_SENSOR_NCS_PORT, &gpio_init);

    board_sensor_bus_deselect();

    RCC_GetClocksFreq(&clocks);
    spi_prescaler =
        board_sensor_select_prescaler(clocks.PCLK2_Frequency);

    if (spi_prescaler == 0U)
    {
        return false;
    }

    SPI_I2S_DeInit(BOARD_SENSOR_SPI);
    SPI_StructInit(&spi_init);

    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode = SPI_Mode_Master;
    spi_init.SPI_DataSize = SPI_DataSize_8b;
    spi_init.SPI_CPOL = SPI_CPOL_High;
    spi_init.SPI_CPHA = SPI_CPHA_2Edge;
    spi_init.SPI_NSS = SPI_NSS_Soft;
    spi_init.SPI_BaudRatePrescaler = spi_prescaler;
    spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
    spi_init.SPI_CRCPolynomial = 7U;

    SPI_Init(BOARD_SENSOR_SPI, &spi_init);
    SPI_NSSInternalSoftwareConfig(
        BOARD_SENSOR_SPI,
        SPI_NSSInternalSoft_Set);
    SPI_Cmd(BOARD_SENSOR_SPI, ENABLE);

    return true;
}

bool board_sensor_bus_transfer(
    const uint8_t *transmit_data,
    uint8_t *receive_data,
    size_t length)
{
    size_t index;

    if ((transmit_data == NULL) &&
        (receive_data == NULL) &&
        (length != 0U))
    {
        return false;
    }

    for (index = 0U; index < length; index++)
    {
        const uint8_t transmit_byte =
            (transmit_data != NULL)
                ? transmit_data[index]
                : 0xFFU;

        if (!board_sensor_wait_for_flag(
                SPI_I2S_FLAG_TXE,
                SET))
        {
            return false;
        }

        SPI_I2S_SendData(
            BOARD_SENSOR_SPI,
            transmit_byte);

        if (!board_sensor_wait_for_flag(
                SPI_I2S_FLAG_RXNE,
                SET))
        {
            return false;
        }

        if (receive_data != NULL)
        {
            receive_data[index] =
                (uint8_t)SPI_I2S_ReceiveData(
                    BOARD_SENSOR_SPI);
        }
        else
        {
            (void)SPI_I2S_ReceiveData(
                BOARD_SENSOR_SPI);
        }
    }

    if (!board_sensor_wait_for_flag(
            SPI_I2S_FLAG_BSY,
            RESET))
    {
        return false;
    }

    return true;
}

void board_sensor_bus_select(void)
{
    GPIO_ResetBits(
        BOARD_SENSOR_NCS_PORT,
        BOARD_SENSOR_NCS_PIN);
}

void board_sensor_bus_deselect(void)
{
    GPIO_SetBits(
        BOARD_SENSOR_NCS_PORT,
        BOARD_SENSOR_NCS_PIN);
}

void board_sensor_bus_delay_ms(uint32_t delay_ms)
{
    const uint32_t start_ms = board_timebase_get_ms();

    while ((board_timebase_get_ms() - start_ms) < delay_ms)
    {
        __NOP();
    }
}
