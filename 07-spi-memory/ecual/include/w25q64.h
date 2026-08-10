#ifndef W25Q64_H
#define W25Q64_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory_types.h"

#define W25Q64_SIZE_BYTES       (8388608UL)
#define W25Q64_PAGE_SIZE_BYTES  (256U)
#define W25Q64_SECTOR_SIZE_BYTES (4096UL)

#define W25Q64_WINBOND_MANUFACTURER_ID (0xEFU)
#define W25Q64_CAPACITY_ID_64MBIT       (0x17U)

bool w25q64_init(memory_jedec_id_t *jedec_id);

bool w25q64_read(uint32_t address,
                 uint8_t *data,
                 size_t length);

bool w25q64_page_program(uint32_t address,
                         const uint8_t *data,
                         size_t length);

bool w25q64_sector_erase(uint32_t address);

bool w25q64_read_status1(uint8_t *status1);

#endif /* W25Q64_H */
