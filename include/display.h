#pragma once

#include <stdint.h>

#define DISPLAY_WIDTH 600
#define DISPLAY_HEIGHT 448

#define EPD_COLOR_BLACK 0x0
#define EPD_COLOR_WHITE 0x1
#define EPD_COLOR_GREEN 0x2
#define EPD_COLOR_BLUE 0x3
#define EPD_COLOR_RED 0x4
#define EPD_COLOR_YELLOW 0x5
#define EPD_COLOR_ORANGE 0x6
#define EPD_COLOR_CLEAR 0x7

void EPD_Clear(uint8_t color);
void EPD_Sleep(void);
void EPD_SendImage(const uint8_t *image);
void EPD_Init(void);
void EPD_SendPartImage(const uint8_t *image, uint16_t xstart, uint16_t ystart,
                       uint16_t image_width, uint16_t image_heigh);
