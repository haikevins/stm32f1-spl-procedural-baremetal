#ifndef APPLICATION_CONFIG_H
#define APPLICATION_CONFIG_H

/*
 * W25Q64 is 8 MiB (0x000000..0x7FFFFF).
 * The demo deliberately uses the last 4 KiB sector.
 *
 * WARNING: this sector is ERASED once at every reset.
 */
#define MEMORY_DEMO_TEST_SECTOR_ADDRESS (0x007FF000UL)
#define MEMORY_DEMO_TEST_LENGTH         (32U)
#define MEMORY_HEARTBEAT_PERIOD_MS      (500UL)

#endif /* APPLICATION_CONFIG_H */
