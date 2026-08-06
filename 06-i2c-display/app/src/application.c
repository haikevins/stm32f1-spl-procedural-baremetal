#include "application.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "application_config.h"
#include "display_service.h"
#include "time_service.h"

#if DISPLAY_DEMO_UPDATE_PERIOD_MS == 0
#error "DISPLAY_DEMO_UPDATE_PERIOD_MS must be greater than zero"
#endif

#if (DISPLAY_DEMO_PROGRESS_STEP == 0) || \
    (DISPLAY_DEMO_PROGRESS_STEP > 100)
#error "DISPLAY_DEMO_PROGRESS_STEP must be in the range 1..100"
#endif

static uint32_t s_last_update_ms;
static uint8_t s_progress_percent;
static bool s_progress_increasing;
static bool s_display_operational;

static void application_format_unsigned(
    uint32_t value,
    char *buffer,
    size_t capacity)
{
    char reversed[10];
    size_t digit_count = 0U;
    size_t output_index;

    if ((buffer == NULL) || (capacity == 0U))
    {
        return;
    }

    do
    {
        reversed[digit_count] =
            (char)('0' + (value % 10U));
        digit_count++;
        value /= 10U;
    }
    while ((value != 0U) &&
           (digit_count < sizeof(reversed)));

    if (digit_count >= capacity)
    {
        digit_count = capacity - 1U;
    }

    for (output_index = 0U;
         output_index < digit_count;
         output_index++)
    {
        buffer[output_index] =
            reversed[digit_count - output_index - 1U];
    }

    buffer[digit_count] = '\0';
}

static void application_render(void)
{
    char seconds_text[11];
    const uint32_t elapsed_seconds =
        time_service_get_ms() / 1000U;

    application_format_unsigned(
        elapsed_seconds,
        seconds_text,
        sizeof(seconds_text));

    display_service_clear();
    display_service_draw_text(0U, 0U, "STM32F103");
    display_service_draw_text(0U, 11U, "I2C SSD1306");
    display_service_draw_text(0U, 27U, "UPTIME");
    display_service_draw_text(48U, 27U, seconds_text);
    display_service_draw_text(0U, 38U, "SECONDS");
    display_service_draw_progress_bar(
        0U,
        52U,
        DISPLAY_SERVICE_WIDTH,
        12U,
        s_progress_percent);
}

static void application_advance_progress(void)
{
    if (s_progress_increasing)
    {
        if (s_progress_percent >=
            (uint8_t)(100U - DISPLAY_DEMO_PROGRESS_STEP))
        {
            s_progress_percent = 100U;
            s_progress_increasing = false;
        }
        else
        {
            s_progress_percent =
                (uint8_t)(s_progress_percent +
                          DISPLAY_DEMO_PROGRESS_STEP);
        }
    }
    else
    {
        if (s_progress_percent <=
            DISPLAY_DEMO_PROGRESS_STEP)
        {
            s_progress_percent = 0U;
            s_progress_increasing = true;
        }
        else
        {
            s_progress_percent =
                (uint8_t)(s_progress_percent -
                          DISPLAY_DEMO_PROGRESS_STEP);
        }
    }
}

bool application_init(void)
{
    s_last_update_ms = time_service_get_ms();
    s_progress_percent = 0U;
    s_progress_increasing = true;
    s_display_operational = true;

    application_render();
    s_display_operational =
        display_service_present();

    return s_display_operational;
}

void application_process(void)
{
    if (!s_display_operational)
    {
        return;
    }

    if (time_service_elapsed_ms(s_last_update_ms) <
        DISPLAY_DEMO_UPDATE_PERIOD_MS)
    {
        return;
    }

    s_last_update_ms += DISPLAY_DEMO_UPDATE_PERIOD_MS;
    application_advance_progress();
    application_render();

    if (!display_service_present())
    {
        s_display_operational = false;
    }
}
