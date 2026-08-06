#include "board_display_bus.h"

#include <stddef.h>
#include <stdint.h>

#include "board_config.h"
#include "board_pins.h"
#include "board_timebase.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"

#define SSD1306_I2C_CONTROL_COMMAND (0x00U)
#define SSD1306_I2C_CONTROL_DATA    (0x40U)

#if (BOARD_DISPLAY_I2C_ADDRESS_7BIT > 0x7FU)
#error "BOARD_DISPLAY_I2C_ADDRESS_7BIT must be a 7-bit address"
#endif

#if (BOARD_DISPLAY_I2C_CLOCK_HZ == 0U) || \
    (BOARD_DISPLAY_I2C_CLOCK_HZ > 400000UL)
#error "BOARD_DISPLAY_I2C_CLOCK_HZ must be in the range 1..400000"
#endif

#if BOARD_DISPLAY_I2C_TIMEOUT_MS == 0U
#error "BOARD_DISPLAY_I2C_TIMEOUT_MS must be greater than zero"
#endif

static bool board_display_i2c_error_pending(void)
{
    return
        (I2C_GetFlagStatus(
             BOARD_DISPLAY_I2C,
             I2C_FLAG_BERR) == SET) ||
        (I2C_GetFlagStatus(
             BOARD_DISPLAY_I2C,
             I2C_FLAG_ARLO) == SET) ||
        (I2C_GetFlagStatus(
             BOARD_DISPLAY_I2C,
             I2C_FLAG_AF) == SET) ||
        (I2C_GetFlagStatus(
             BOARD_DISPLAY_I2C,
             I2C_FLAG_OVR) == SET) ||
        (I2C_GetFlagStatus(
             BOARD_DISPLAY_I2C,
             I2C_FLAG_TIMEOUT) == SET);
}

static void board_display_i2c_clear_errors(void)
{
    if (I2C_GetFlagStatus(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_BERR) == SET)
    {
        I2C_ClearFlag(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_BERR);
    }

    if (I2C_GetFlagStatus(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_ARLO) == SET)
    {
        I2C_ClearFlag(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_ARLO);
    }

    if (I2C_GetFlagStatus(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_AF) == SET)
    {
        I2C_ClearFlag(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_AF);
    }

    if (I2C_GetFlagStatus(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_OVR) == SET)
    {
        I2C_ClearFlag(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_OVR);
    }

    if (I2C_GetFlagStatus(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_TIMEOUT) == SET)
    {
        I2C_ClearFlag(
            BOARD_DISPLAY_I2C,
            I2C_FLAG_TIMEOUT);
    }
}

static bool board_display_wait_for_event(uint32_t event)
{
    const uint32_t start_ms = board_timebase_get_ms();

    while (I2C_CheckEvent(
               BOARD_DISPLAY_I2C,
               event) != SUCCESS)
    {
        if (board_display_i2c_error_pending())
        {
            return false;
        }

        if ((board_timebase_get_ms() - start_ms) >=
            BOARD_DISPLAY_I2C_TIMEOUT_MS)
        {
            return false;
        }
    }

    return true;
}

static bool board_display_wait_until_bus_idle(void)
{
    const uint32_t start_ms = board_timebase_get_ms();

    while (I2C_GetFlagStatus(
               BOARD_DISPLAY_I2C,
               I2C_FLAG_BUSY) == SET)
    {
        if ((board_timebase_get_ms() - start_ms) >=
            BOARD_DISPLAY_I2C_TIMEOUT_MS)
        {
            return false;
        }
    }

    return true;
}

static void board_display_abort_transfer(void)
{
    I2C_GenerateSTOP(
        BOARD_DISPLAY_I2C,
        ENABLE);
    board_display_i2c_clear_errors();
}

bool board_display_bus_init(void)
{
    GPIO_InitTypeDef gpio_init;
    I2C_InitTypeDef i2c_init;

    RCC_APB2PeriphClockCmd(
        BOARD_DISPLAY_GPIO_CLOCK,
        ENABLE);
    RCC_APB1PeriphClockCmd(
        BOARD_DISPLAY_I2C_CLOCK,
        ENABLE);

    gpio_init.GPIO_Pin =
        BOARD_DISPLAY_SCL_PIN |
        BOARD_DISPLAY_SDA_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(
        BOARD_DISPLAY_GPIO_PORT,
        &gpio_init);

    I2C_DeInit(BOARD_DISPLAY_I2C);
    I2C_StructInit(&i2c_init);

    i2c_init.I2C_ClockSpeed =
        BOARD_DISPLAY_I2C_CLOCK_HZ;
    i2c_init.I2C_Mode = I2C_Mode_I2C;
    i2c_init.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c_init.I2C_OwnAddress1 = 0U;
    i2c_init.I2C_Ack = I2C_Ack_Enable;
    i2c_init.I2C_AcknowledgedAddress =
        I2C_AcknowledgedAddress_7bit;

    I2C_Init(
        BOARD_DISPLAY_I2C,
        &i2c_init);
    I2C_Cmd(
        BOARD_DISPLAY_I2C,
        ENABLE);

    board_display_i2c_clear_errors();

    return board_display_wait_until_bus_idle();
}

bool board_display_bus_write(
    bool data_mode,
    const uint8_t *data,
    size_t length)
{
    const uint8_t control_byte =
        data_mode
            ? SSD1306_I2C_CONTROL_DATA
            : SSD1306_I2C_CONTROL_COMMAND;
    const uint8_t shifted_address =
        (uint8_t)(BOARD_DISPLAY_I2C_ADDRESS_7BIT << 1U);
    size_t index;

    if ((data == NULL) && (length != 0U))
    {
        return false;
    }

    if (length == 0U)
    {
        return true;
    }

    board_display_i2c_clear_errors();

    if (!board_display_wait_until_bus_idle())
    {
        return false;
    }

    I2C_GenerateSTART(
        BOARD_DISPLAY_I2C,
        ENABLE);

    if (!board_display_wait_for_event(
            I2C_EVENT_MASTER_MODE_SELECT))
    {
        board_display_abort_transfer();
        return false;
    }

    I2C_Send7bitAddress(
        BOARD_DISPLAY_I2C,
        shifted_address,
        I2C_Direction_Transmitter);

    if (!board_display_wait_for_event(
            I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        board_display_abort_transfer();
        return false;
    }

    I2C_SendData(
        BOARD_DISPLAY_I2C,
        control_byte);

    for (index = 0U; index < length; index++)
    {
        if (!board_display_wait_for_event(
                I2C_EVENT_MASTER_BYTE_TRANSMITTING))
        {
            board_display_abort_transfer();
            return false;
        }

        I2C_SendData(
            BOARD_DISPLAY_I2C,
            data[index]);
    }

    if (!board_display_wait_for_event(
            I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        board_display_abort_transfer();
        return false;
    }

    I2C_GenerateSTOP(
        BOARD_DISPLAY_I2C,
        ENABLE);

    return true;
}

void board_display_bus_delay_ms(uint32_t delay_ms)
{
    const uint32_t start_ms = board_timebase_get_ms();

    while ((board_timebase_get_ms() - start_ms) < delay_ms)
    {
        __NOP();
    }
}
