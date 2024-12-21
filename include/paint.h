#pragma once

#include "communication.h"
#include "fonts.h"

/**
 * Image attributes
 **/
typedef struct {
  uint8_t *Image;
  uint16_t Width;
  uint16_t Height;
  uint16_t WidthMemory;
  uint16_t HeightMemory;
  uint16_t Color;
  uint16_t Rotate;
  uint16_t Mirror;
  uint16_t WidthByte;
  uint16_t HeightByte;
} PAINT;
extern PAINT Paint;

/**
 * Display rotate
 **/
#define ROTATE_0 0
#define ROTATE_90 90
#define ROTATE_180 180
#define ROTATE_270 270

/**
 * Display Flip
 **/
typedef enum {
  MIRROR_NONE = 0x00,
  MIRROR_HORIZONTAL = 0x01,
  MIRROR_VERTICAL = 0x02,
  MIRROR_ORIGIN = 0x03,
} MIRROR_IMAGE;
#define MIRROR_IMAGE_DFT MIRROR_NONE

#define NO_BACKGROUND 0xAA

// 4 Gray level
#define GRAY1 0x03 // Blackest
#define GRAY2 0x02
#define GRAY3 0x01 // gray
#define GRAY4 0x00 // white
/**
 * The size of the point
 **/
typedef enum {
  DOT_PIXEL_1X1 = 1, // 1 x 1
  DOT_PIXEL_2X2,     // 2 X 2
  DOT_PIXEL_3X3,     // 3 X 3
  DOT_PIXEL_4X4,     // 4 X 4
  DOT_PIXEL_5X5,     // 5 X 5
  DOT_PIXEL_6X6,     // 6 X 6
  DOT_PIXEL_7X7,     // 7 X 7
  DOT_PIXEL_8X8,     // 8 X 8
} DOT_PIXEL;
#define DOT_PIXEL_DFT DOT_PIXEL_1X1 // Default dot pilex

/**
 * Point size fill style
 **/
typedef enum {
  DOT_FILL_AROUND = 1, // dot pixel 1 x 1
  DOT_FILL_RIGHTUP,    // dot pixel 2 X 2
} DOT_STYLE;
#define DOT_STYLE_DFT DOT_FILL_AROUND // Default dot pilex

/**
 * Line style, solid or dashed
 **/
typedef enum {
  LINE_STYLE_SOLID = 0,
  LINE_STYLE_DOTTED,
} LINE_STYLE;

/**
 * Whether the graphic is filled
 **/
typedef enum {
  DRAW_FILL_EMPTY = 0,
  DRAW_FILL_FULL,
} DRAW_FILL;

// init and Clear
void Paint_NewImage(uint8_t *image, uint16_t Width, uint16_t Height,
                    uint16_t Rotate, uint16_t Color);
void Paint_SelectImage(uint8_t *image);
void Paint_SetRotate(uint16_t Rotate);
void Paint_SetMirroring(uint8_t mirror);
void Paint_SetPixel(uint16_t Xpoint, uint16_t Ypoint, uint16_t Color);

void Paint_Clear(uint16_t Color);
void Paint_ClearWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend,
                        uint16_t Yend, uint16_t Color);

// Drawing
void Paint_DrawPoint(uint16_t Xpoint, uint16_t Ypoint, uint16_t Color,
                     DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_FillWay);
void Paint_DrawLine(uint16_t Xstart, uint16_t Ystart, uint16_t Xend,
                    uint16_t Yend, uint16_t Color, DOT_PIXEL Line_width,
                    LINE_STYLE Line_Style);
void Paint_DrawRectangle(uint16_t Xstart, uint16_t Ystart, uint16_t Xend,
                         uint16_t Yend, uint16_t Color, DOT_PIXEL Line_width,
                         DRAW_FILL Draw_Fill);
void Paint_DrawCircle(uint16_t X_Center, uint16_t Y_Center, uint16_t Radius,
                      uint16_t Color, DOT_PIXEL Line_width,
                      DRAW_FILL Draw_Fill);

// Display string
void Paint_DrawChar(uint16_t Xstart, uint16_t Ystart, const char Acsii_Char,
                    FONT *Font, uint16_t Color_Foreground,
                    uint16_t Color_Background);
void Paint_DrawString(uint16_t Xstart, uint16_t Ystart, const char *pString,
                      FONT *Font, uint16_t Color_Foreground,
                      uint16_t Color_Background);
void Paint_DrawNum(uint16_t Xpoint, uint16_t Ypoint, int32_t Nummber,
                   FONT *Font, uint16_t Color_Foreground,
                   uint16_t Color_Background);

// pic
void Paint_DrawBitMap(const unsigned char *image_buffer);
void Paint_DrawImage(const unsigned char *image_buffer, uint16_t xStart,
                     uint16_t yStart, uint16_t W_Image, uint16_t H_Image);
