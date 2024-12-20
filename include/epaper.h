#pragma once
#include "epaper_dev.h"

/**********************************
Color Index
**********************************/
#define EPD_5IN65F_BLACK 0x0  /// 000
#define EPD_5IN65F_WHITE 0x1  ///	001
#define EPD_5IN65F_GREEN 0x2  ///	010
#define EPD_5IN65F_BLUE 0x3   ///	011
#define EPD_5IN65F_RED 0x4    ///	100
#define EPD_5IN65F_YELLOW 0x5 ///	101
#define EPD_5IN65F_ORANGE 0x6 ///	110
#define EPD_5IN65F_CLEAN 0x7  ///	111   unavailable  Afterimage

#define EPD_5IN65F_WIDTH 600
#define EPD_5IN65F_HEIGHT 448

void EPD_5IN65F_Clear(UBYTE color);
void EPD_5IN65F_Sleep(void);
void EPD_5IN65F_Display(const UBYTE *image);
void EPD_5IN65F_DisplayFullImage(const UBYTE *image);
void EPD_5IN65F_Init(void);
void EPD_5IN65F_Display_part(const UBYTE *image, UWORD xstart, UWORD ystart,
                             UWORD image_width, UWORD image_heigh);
