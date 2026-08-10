#include "board_memory_bus.h"

#include "board_config.h"
#include "board_pins.h"
#include "board_timebase.h"
#include "stm32f10x.h"

#if BOARD_MEMORY_SPI_MAX_HZ == 0
#error "BOARD_MEMORY_SPI_MAX_HZ must be greater than zero"
#endif

#if BOARD_MEMORY_SPI_TIMEOUT_MS == 0
#error "BOARD_MEMORY_SPI_TIMEOUT_MS must be greater than zero"
#endif

static uint16_t board_memory_select_prescaler(
    uint32_t peripheral_clock_hz)
{
    if (((peripheral_clock_hz + 1U) / 2U) <=
        BOARD_MEMORY_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_2;
    }

    if (((peripheral_clock_hz + 3U) / 4U) <=
        BOARD_MEMORY_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_4;
    }

    if (((peripheral_clock_hz + 7U) / 8U) <=
        BOARD_MEMORY_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_8;
    }

    if (((peripheral_clock_hz + 15U) / 16U) <=
        BOARD_MEMORY_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_16;
    }

    if (((peripheral_clock_hz + 31U) / 32U) <=
        BOARD_MEMORY_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_32;
    }

    if (((peripheral_clock_hz + 63U) / 64U) <=
        BOARD_MEMORY_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_64;
    }

    if (((peripheral_clock_hz + 127U) / 128U) <=
        BOARD_MEMORY_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_128;
    }

    if (((peripheral_clock_hz + 255U) / 256U) <=
        BOARD_MEMORY_SPI_MAX_HZ)
    {
        return SPI_BaudRatePrescaler_256;
    }

    return 0U;
}

static bool board_memory_wait_for_flag(
    uint16_t flag,
    FlagStatus expected_status)
{
    const uint32_t start_ms = board_timebase_get_ms();

    while (SPI_I2S_GetFlagStatus(
               BOARD_MEMORY_SPI,
               flag) != expected_status)
    {
        if ((board_timebase_get_ms() - start_ms) >=
            BOARD_MEMORY_SPI_TIMEOUT_MS)
        {
            return false;
        }
    }

    return true;
}

bool board_memory_bus_init(void)
{
    GPIO_InitTypeDef gpio_init;
    SPI_InitTypeDef spi_init;
    RCC_ClocksTypeDef clocks;
    uint16_t spi_prescaler;

    RCC_APB2PeriphClockCmd(
        BOARD_MEMORY_GPIO_CLOCK |
        BOARD_MEMORY_SPI_CLOCK,
        ENABLE);

    /*
     * Drive CS high before turning it into an output. W25Q devices require
     * a high-to-low transition after power-up before accepting a command.
     */
    GPIO_SetBits(
        BOARD_MEMORY_CS_PORT,
        BOARD_MEMORY_CS_PIN);

    gpio_init.GPIO_Pin = BOARD_MEMORY_CS_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(BOARD_MEMORY_CS_PORT, &gpio_init);

    gpio_init.GPIO_Pin =
        BOARD_MEMORY_SCK_PIN |
        BOARD_MEMORY_MOSI_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(BOARD_MEMORY_SCK_PORT, &gpio_init);

    gpio_init.GPIO_Pin = BOARD_MEMORY_MISO_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(BOARD_MEMORY_MISO_PORT, &gpio_init);

    board_memory_bus_deselect();

    RCC_GetClocksFreq(&clocks);
    spi_prescaler =
        board_memory_select_prescaler(clocks.PCLK2_Frequency);

    if (spi_prescaler == 0U)
    {
        return false;
    }

    SPI_I2S_DeInit(BOARD_MEMORY_SPI);
    SPI_StructInit(&spi_init);

    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode = SPI_Mode_Master;
    spi_init.SPI_DataSize = SPI_DataSize_8b;

    /*
     * W25Q64 supports standard SPI mode 0 and mode 3.
     * Use mode 0 here: CPOL=0, CPHA=0.
     */
    spi_init.SPI_CPOL = SPI_CPOL_Low;
    spi_init.SPI_CPHA = SPI_CPHA_1Edge;

    spi_init.SPI_NSS = SPI_NSS_Soft;
    spi_init.SPI_BaudRatePrescaler = spi_prescaler;
    spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
    spi_init.SPI_CRCPolynomial = 7U;

    SPI_Init(BOARD_MEMORY_SPI, &spi_init);
    SPI_NSSInternalSoftwareConfig(
        BOARD_MEMORY_SPI,
        SPI_NSSInternalSoft_Set);
    SPI_Cmd(BOARD_MEMORY_SPI, ENABLE);

    return true;
}

bool board_memory_bus_transfer(
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

        if (!board_memory_wait_for_flag(
                SPI_I2S_FLAG_TXE,
                SET))
        {
            return false;
        }

        SPI_I2S_SendData(
            BOARD_MEMORY_SPI,
            transmit_byte);

        if (!board_memory_wait_for_flag(
                SPI_I2S_FLAG_RXNE,
                SET))
        {
            return false;
        }

        if (receive_data != NULL)
        {
            receive_data[index] =
                (uint8_t)SPI_I2S_ReceiveData(
                    BOARD_MEMORY_SPI);
        }
        else
        {
            (void)SPI_I2S_ReceiveData(
                BOARD_MEMORY_SPI);
        }
    }

    if (!board_memory_wait_for_flag(
            SPI_I2S_FLAG_BSY,
            RESET))
    {
        return false;
    }

    return true;
}

void board_memory_bus_select(void)
{
    GPIO_ResetBits(
        BOARD_MEMORY_CS_PORT,
        BOARD_MEMORY_CS_PIN);
}

void board_memory_bus_deselect(void)
{
    GPIO_SetBits(
        BOARD_MEMORY_CS_PORT,
        BOARD_MEMORY_CS_PIN);
}

void board_memory_bus_delay_ms(uint32_t delay_ms)
{
    const uint32_t start_ms = board_timebase_get_ms();

    while ((board_timebase_get_ms() - start_ms) < delay_ms)
    {
        __NOP();
    }
}
