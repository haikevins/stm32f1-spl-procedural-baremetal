#include "w25q64.h"

#include <stddef.h>

#include "board_config.h"
#include "board_memory_bus.h"
#include "board_timebase.h"

#define W25Q64_CMD_WRITE_ENABLE       (0x06U)
#define W25Q64_CMD_READ_STATUS1       (0x05U)
#define W25Q64_CMD_READ_DATA          (0x03U)
#define W25Q64_CMD_PAGE_PROGRAM       (0x02U)
#define W25Q64_CMD_SECTOR_ERASE_4K    (0x20U)
#define W25Q64_CMD_JEDEC_ID           (0x9FU)

#define W25Q64_STATUS1_BUSY           (0x01U)
#define W25Q64_STATUS1_WEL            (0x02U)

static bool w25q64_address_range_valid(
    uint32_t address,
    size_t length)
{
    if (length == 0U)
    {
        return address <= W25Q64_SIZE_BYTES;
    }

    if (address >= W25Q64_SIZE_BYTES)
    {
        return false;
    }

    return length <=
        (size_t)(W25Q64_SIZE_BYTES - address);
}

static void w25q64_encode_address(
    uint32_t address,
    uint8_t *bytes)
{
    bytes[0] = (uint8_t)(address >> 16U);
    bytes[1] = (uint8_t)(address >> 8U);
    bytes[2] = (uint8_t)address;
}

static bool w25q64_command_only(uint8_t command)
{
    bool ok;

    board_memory_bus_select();
    ok = board_memory_bus_transfer(
        &command,
        NULL,
        1U);
    board_memory_bus_deselect();

    return ok;
}

bool w25q64_read_status1(uint8_t *status1)
{
    const uint8_t command = W25Q64_CMD_READ_STATUS1;
    bool ok;

    if (status1 == NULL)
    {
        return false;
    }

    board_memory_bus_select();

    ok = board_memory_bus_transfer(
        &command,
        NULL,
        1U);

    if (ok)
    {
        ok = board_memory_bus_transfer(
            NULL,
            status1,
            1U);
    }

    board_memory_bus_deselect();

    return ok;
}

static bool w25q64_wait_ready(uint32_t timeout_ms)
{
    const uint32_t start_ms = board_timebase_get_ms();

    for (;;)
    {
        uint8_t status1;

        if (!w25q64_read_status1(&status1))
        {
            return false;
        }

        if ((status1 & W25Q64_STATUS1_BUSY) == 0U)
        {
            return true;
        }

        if ((board_timebase_get_ms() - start_ms) >= timeout_ms)
        {
            return false;
        }
    }
}

static bool w25q64_write_enable(void)
{
    uint8_t status1;

    if (!w25q64_command_only(W25Q64_CMD_WRITE_ENABLE))
    {
        return false;
    }

    if (!w25q64_read_status1(&status1))
    {
        return false;
    }

    return (status1 & W25Q64_STATUS1_WEL) != 0U;
}

bool w25q64_init(memory_jedec_id_t *jedec_id)
{
    const uint8_t command = W25Q64_CMD_JEDEC_ID;
    uint8_t id[3];
    bool ok;

    if (jedec_id == NULL)
    {
        return false;
    }

    jedec_id->manufacturer_id = 0U;
    jedec_id->memory_type = 0U;
    jedec_id->capacity_id = 0U;

    if (!w25q64_wait_ready(
            BOARD_MEMORY_PAGE_PROGRAM_TIMEOUT_MS))
    {
        return false;
    }

    board_memory_bus_select();

    ok = board_memory_bus_transfer(
        &command,
        NULL,
        1U);

    if (ok)
    {
        ok = board_memory_bus_transfer(
            NULL,
            id,
            sizeof(id));
    }

    board_memory_bus_deselect();

    if (!ok)
    {
        return false;
    }

    jedec_id->manufacturer_id = id[0];
    jedec_id->memory_type = id[1];
    jedec_id->capacity_id = id[2];

    /*
     * Do not require one exact memory-type byte so the driver remains useful
     * across W25Q64 revisions. Manufacturer EFh + density code 17h identifies
     * the expected Winbond 64-Mbit class for this example.
     */
    return
        (id[0] == W25Q64_WINBOND_MANUFACTURER_ID) &&
        (id[2] == W25Q64_CAPACITY_ID_64MBIT);
}

bool w25q64_read(uint32_t address,
                 uint8_t *data,
                 size_t length)
{
    uint8_t header[4];
    bool ok;

    if (((data == NULL) && (length != 0U)) ||
        !w25q64_address_range_valid(address, length))
    {
        return false;
    }

    if (length == 0U)
    {
        return true;
    }

    if (!w25q64_wait_ready(
            BOARD_MEMORY_PAGE_PROGRAM_TIMEOUT_MS))
    {
        return false;
    }

    header[0] = W25Q64_CMD_READ_DATA;
    w25q64_encode_address(address, &header[1]);

    board_memory_bus_select();

    ok = board_memory_bus_transfer(
        header,
        NULL,
        sizeof(header));

    if (ok)
    {
        ok = board_memory_bus_transfer(
            NULL,
            data,
            length);
    }

    board_memory_bus_deselect();

    return ok;
}

bool w25q64_page_program(uint32_t address,
                         const uint8_t *data,
                         size_t length)
{
    uint8_t header[4];
    const uint32_t page_offset =
        address % W25Q64_PAGE_SIZE_BYTES;
    bool ok;

    if ((data == NULL) ||
        (length == 0U) ||
        (length > W25Q64_PAGE_SIZE_BYTES) ||
        !w25q64_address_range_valid(address, length) ||
        ((page_offset + length) > W25Q64_PAGE_SIZE_BYTES))
    {
        return false;
    }

    if (!w25q64_wait_ready(
            BOARD_MEMORY_PAGE_PROGRAM_TIMEOUT_MS))
    {
        return false;
    }

    if (!w25q64_write_enable())
    {
        return false;
    }

    header[0] = W25Q64_CMD_PAGE_PROGRAM;
    w25q64_encode_address(address, &header[1]);

    board_memory_bus_select();

    ok = board_memory_bus_transfer(
        header,
        NULL,
        sizeof(header));

    if (ok)
    {
        ok = board_memory_bus_transfer(
            data,
            NULL,
            length);
    }

    board_memory_bus_deselect();

    if (!ok)
    {
        return false;
    }

    return w25q64_wait_ready(
        BOARD_MEMORY_PAGE_PROGRAM_TIMEOUT_MS);
}

bool w25q64_sector_erase(uint32_t address)
{
    uint8_t command[4];

    if (address >= W25Q64_SIZE_BYTES)
    {
        return false;
    }

    address -= address % W25Q64_SECTOR_SIZE_BYTES;

    if (!w25q64_wait_ready(
            BOARD_MEMORY_PAGE_PROGRAM_TIMEOUT_MS))
    {
        return false;
    }

    if (!w25q64_write_enable())
    {
        return false;
    }

    command[0] = W25Q64_CMD_SECTOR_ERASE_4K;
    w25q64_encode_address(address, &command[1]);

    board_memory_bus_select();

    if (!board_memory_bus_transfer(
            command,
            NULL,
            sizeof(command)))
    {
        board_memory_bus_deselect();
        return false;
    }

    board_memory_bus_deselect();

    return w25q64_wait_ready(
        BOARD_MEMORY_SECTOR_ERASE_TIMEOUT_MS);
}
