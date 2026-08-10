#include "memory_service.h"

#include "w25q64.h"

static memory_jedec_id_t s_jedec_id;

bool memory_service_init(void)
{
    s_jedec_id.manufacturer_id = 0U;
    s_jedec_id.memory_type = 0U;
    s_jedec_id.capacity_id = 0U;

    return w25q64_init(&s_jedec_id);
}

memory_jedec_id_t memory_service_get_jedec_id(void)
{
    return s_jedec_id;
}

bool memory_service_read(uint32_t address,
                         uint8_t *data,
                         size_t length)
{
    return w25q64_read(address, data, length);
}

bool memory_service_program_page(uint32_t address,
                                 const uint8_t *data,
                                 size_t length)
{
    return w25q64_page_program(address, data, length);
}

bool memory_service_erase_sector(uint32_t address)
{
    return w25q64_sector_erase(address);
}
