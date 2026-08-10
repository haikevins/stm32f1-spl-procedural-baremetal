#include "application.h"

#include <stddef.h>

#include "application_config.h"
#include "indication_service.h"
#include "memory_service.h"
#include "memory_types.h"
#include "time_service.h"

#if MEMORY_DEMO_TEST_LENGTH == 0
#error "MEMORY_DEMO_TEST_LENGTH must be greater than zero"
#endif

#if MEMORY_DEMO_TEST_LENGTH > 256
#error "MEMORY_DEMO_TEST_LENGTH must fit one W25Q64 page"
#endif

#if MEMORY_HEARTBEAT_PERIOD_MS == 0
#error "MEMORY_HEARTBEAT_PERIOD_MS must be greater than zero"
#endif

static const uint8_t s_test_pattern[MEMORY_DEMO_TEST_LENGTH] =
{
    0x53U, 0x54U, 0x4DU, 0x33U, 0x32U, 0x46U, 0x31U, 0x30U,
    0x33U, 0x20U, 0x57U, 0x32U, 0x35U, 0x51U, 0x36U, 0x34U,
    0x20U, 0x53U, 0x50U, 0x49U, 0x20U, 0x4DU, 0x45U, 0x4DU,
    0x4FU, 0x52U, 0x59U, 0x20U, 0x4FU, 0x4BU, 0x21U, 0xA5U
};

static uint8_t s_readback[MEMORY_DEMO_TEST_LENGTH];
static uint32_t s_last_heartbeat_ms;

volatile uint8_t application_memory_manufacturer_id;
volatile uint8_t application_memory_type_id;
volatile uint8_t application_memory_capacity_id;

volatile uint32_t application_memory_test_address;
volatile uint32_t application_memory_test_sequence;
volatile uint32_t application_memory_error_count;

volatile bool application_memory_erase_ok;
volatile bool application_memory_program_ok;
volatile bool application_memory_verify_ok;
volatile bool application_memory_test_passed;

volatile uint8_t application_memory_first_mismatch_index;
volatile uint8_t application_memory_readback_first_byte;
volatile uint8_t application_memory_readback_last_byte;

static bool application_verify_readback(void)
{
    size_t index;

    application_memory_first_mismatch_index = 0xFFU;

    for (index = 0U; index < MEMORY_DEMO_TEST_LENGTH; index++)
    {
        if (s_readback[index] != s_test_pattern[index])
        {
            application_memory_first_mismatch_index =
                (uint8_t)index;
            return false;
        }
    }

    return true;
}

bool application_init(void)
{
    const memory_jedec_id_t id =
        memory_service_get_jedec_id();

    application_memory_manufacturer_id =
        id.manufacturer_id;
    application_memory_type_id =
        id.memory_type;
    application_memory_capacity_id =
        id.capacity_id;

    application_memory_test_address =
        MEMORY_DEMO_TEST_SECTOR_ADDRESS;
    application_memory_test_sequence = 0U;
    application_memory_error_count = 0U;

    application_memory_erase_ok = false;
    application_memory_program_ok = false;
    application_memory_verify_ok = false;
    application_memory_test_passed = false;

    application_memory_first_mismatch_index = 0xFFU;
    application_memory_readback_first_byte = 0U;
    application_memory_readback_last_byte = 0U;

    indication_service_set(
        INDICATION_STATUS,
        false);

    /*
     * Destructive demo: erase the configured sector once, program one page
     * fragment, then read it back byte-for-byte.
     */
    application_memory_erase_ok =
        memory_service_erase_sector(
            MEMORY_DEMO_TEST_SECTOR_ADDRESS);

    if (!application_memory_erase_ok)
    {
        application_memory_error_count++;
        return true;
    }

    application_memory_program_ok =
        memory_service_program_page(
            MEMORY_DEMO_TEST_SECTOR_ADDRESS,
            s_test_pattern,
            sizeof(s_test_pattern));

    if (!application_memory_program_ok)
    {
        application_memory_error_count++;
        return true;
    }

    if (!memory_service_read(
            MEMORY_DEMO_TEST_SECTOR_ADDRESS,
            s_readback,
            sizeof(s_readback)))
    {
        application_memory_error_count++;
        return true;
    }

    application_memory_readback_first_byte =
        s_readback[0];
    application_memory_readback_last_byte =
        s_readback[MEMORY_DEMO_TEST_LENGTH - 1U];

    application_memory_verify_ok =
        application_verify_readback();

    if (!application_memory_verify_ok)
    {
        application_memory_error_count++;
        return true;
    }

    application_memory_test_sequence = 1U;
    application_memory_test_passed = true;
    s_last_heartbeat_ms = time_service_get_ms();

    return true;
}

void application_process(void)
{
    const uint32_t now_ms = time_service_get_ms();

    if (!application_memory_test_passed)
    {
        /*
         * A steady ON LED is the visible failure indication.
         */
        indication_service_set(
            INDICATION_STATUS,
            true);
        return;
    }

    if ((now_ms - s_last_heartbeat_ms) >=
        MEMORY_HEARTBEAT_PERIOD_MS)
    {
        s_last_heartbeat_ms += MEMORY_HEARTBEAT_PERIOD_MS;
        indication_service_toggle(
            INDICATION_STATUS);
    }
}
