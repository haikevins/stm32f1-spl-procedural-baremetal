#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_TIMEBASE_HZ                  (1000UL)

/*
 * Keep the breadboard SPI clock deliberately conservative.
 * With the normal 72 MHz PCLK2 this selects /16 = 4.5 MHz.
 */
#define BOARD_MEMORY_SPI_MAX_HZ            (5000000UL)
#define BOARD_MEMORY_SPI_TIMEOUT_MS        (20UL)
#define BOARD_MEMORY_POWER_ON_DELAY_MS     (10UL)

/*
 * Internal program and erase operations are polled through Status Register-1.
 */
#define BOARD_MEMORY_PAGE_PROGRAM_TIMEOUT_MS (50UL)
#define BOARD_MEMORY_SECTOR_ERASE_TIMEOUT_MS (2000UL)

#endif /* BOARD_CONFIG_H */
