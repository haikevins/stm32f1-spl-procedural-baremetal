#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stdint.h>

#define SSD1306_WIDTH  (128U)
#define SSD1306_HEIGHT (64U)

bool ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_draw_pixel(
    uint8_t x,
    uint8_t y,
    bool illuminated);
void ssd1306_draw_text(
    uint8_t x,
    uint8_t y,
    const char *text);
void ssd1306_draw_progress_bar(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height,
    uint8_t percent);
bool ssd1306_update(void);

#endif /* SSD1306_H */
