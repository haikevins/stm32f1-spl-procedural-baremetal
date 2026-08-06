#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_TIMEBASE_HZ                    (1000UL)

/*
 * SSD1306 I2C modules commonly use 7-bit address 0x3C.
 * Change this to 0x3D if an address scan reports 0x3D.
 */
#define BOARD_DISPLAY_I2C_ADDRESS_7BIT       (0x3CU)
#define BOARD_DISPLAY_I2C_CLOCK_HZ           (400000UL)
#define BOARD_DISPLAY_I2C_TIMEOUT_MS         (20UL)
#define BOARD_DISPLAY_POWER_ON_DELAY_MS      (100UL)

#endif /* BOARD_CONFIG_H */
