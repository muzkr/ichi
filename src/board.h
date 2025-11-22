
#ifndef _BOARD_H
#define _BOARD_H

#include <stdint.h>
#include <stdbool.h>

#define BOARD_DEFAULT_BACKLIGHT_DELAY 20000 // in ms

void board_init();

void board_backlight_on(uint32_t delay);
void board_backlight_off();
void board_backlight_flash(uint32_t delay);
void board_backlight_update();

#endif
