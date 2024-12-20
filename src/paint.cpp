#include "paint.h"
#include "epaper_dev.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h> //memset()

PAINT Paint;

/******************************************************************************
function: Create Image
parameter:
    image   :   Pointer to the image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    Color   :   Whether the picture is inverted
******************************************************************************/
void Paint_NewImage(UBYTE *image, UWORD Width, UWORD Height, UWORD Rotate,
                    UWORD Color) {
  Paint.Image = NULL;
  Paint.Image = image;

  Paint.WidthMemory = Width;
  Paint.HeightMemory = Height;
  Paint.Color = Color;
  Paint.Scale = 2;
  Paint.WidthByte = (Width % 8 == 0) ? (Width / 8) : (Width / 8 + 1);
  Paint.HeightByte = Height;
  //    printf("WidthByte = %d, HeightByte = %d\r\n", Paint.WidthByte,
  //    Paint.HeightByte); printf(" EPD_WIDTH / 8 = %d\r\n",  122 / 8);

  Paint.Rotate = Rotate;
  Paint.Mirror = MIRROR_NONE;

  if (Rotate == ROTATE_0 || Rotate == ROTATE_180) {
    Paint.Width = Width;
    Paint.Height = Height;
  } else {
    Paint.Width = Height;
    Paint.Height = Width;
  }
}

/******************************************************************************
function: Select Image
parameter:
    image : Pointer to the image cache
******************************************************************************/
void Paint_SelectImage(UBYTE *image) { Paint.Image = image; }

/******************************************************************************
function: Select Image Rotate
parameter:
    Rotate : 0,90,180,270
******************************************************************************/
void Paint_SetRotate(UWORD Rotate) {
  if (Rotate == ROTATE_0 || Rotate == ROTATE_90 || Rotate == ROTATE_180 ||
      Rotate == ROTATE_270) {
    Paint.Rotate = Rotate;
  }
}

/******************************************************************************
function:	Select Image mirror
parameter:
    mirror   :Not mirror,Horizontal mirror,Vertical mirror,Origin mirror
******************************************************************************/
void Paint_SetMirroring(UBYTE mirror) {
  if (mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL ||
      mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
    Paint.Mirror = mirror;
  } else {
  }
}

void Paint_SetScale(UBYTE scale) {
  if (scale == 2) {
    Paint.Scale = scale;
    Paint.WidthByte = (Paint.WidthMemory % 8 == 0)
                          ? (Paint.WidthMemory / 8)
                          : (Paint.WidthMemory / 8 + 1);
  } else if (scale == 4) {
    Paint.Scale = scale;
    Paint.WidthByte = (Paint.WidthMemory % 4 == 0)
                          ? (Paint.WidthMemory / 4)
                          : (Paint.WidthMemory / 4 + 1);
  } else if (scale == 7) { // Only applicable with 5in65 e-Paper
    Paint.Scale = 7;
    Paint.WidthByte = (Paint.WidthMemory % 2 == 0)
                          ? (Paint.WidthMemory / 2)
                          : (Paint.WidthMemory / 2 + 1);
  } else {
  }
}
/******************************************************************************
function: Draw Pixels
parameter:
    Xpoint : At point X
    Ypoint : At point Y
    Color  : Painted colors
******************************************************************************/
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color) {
  if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
    return;
  }
  UWORD X, Y;
  switch (Paint.Rotate) {
  case 0:
    X = Xpoint;
    Y = Ypoint;
    break;
  case 90:
    X = Paint.WidthMemory - Ypoint - 1;
    Y = Xpoint;
    break;
  case 180:
    X = Paint.WidthMemory - Xpoint - 1;
    Y = Paint.HeightMemory - Ypoint - 1;
    break;
  case 270:
    X = Ypoint;
    Y = Paint.HeightMemory - Xpoint - 1;
    break;
  default:
    return;
  }

  switch (Paint.Mirror) {
  case MIRROR_NONE:
    break;
  case MIRROR_HORIZONTAL:
    X = Paint.WidthMemory - X - 1;
    break;
  case MIRROR_VERTICAL:
    Y = Paint.HeightMemory - Y - 1;
    break;
  case MIRROR_ORIGIN:
    X = Paint.WidthMemory - X - 1;
    Y = Paint.HeightMemory - Y - 1;
    break;
  default:
    return;
  }

  if (X > Paint.WidthMemory || Y > Paint.HeightMemory) {
    return;
  }

  if (Paint.Scale == 2) {
    UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
    UBYTE Rdata = Paint.Image[Addr];
    if (Color == BLACK)
      Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
    else
      Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
  } else if (Paint.Scale == 4) {
    UDOUBLE Addr = X / 4 + Y * Paint.WidthByte;
    Color = Color % 4; // Guaranteed color scale is 4  --- 0~3
    UBYTE Rdata = Paint.Image[Addr];

    Rdata = Rdata & (~(0xC0 >> ((X % 4) * 2)));
    Paint.Image[Addr] = Rdata | ((Color << 6) >> ((X % 4) * 2));
  } else if (Paint.Scale == 7 || Paint.Scale == 16) {
    UDOUBLE Addr = X / 2 + Y * Paint.WidthByte;
    UBYTE Rdata = Paint.Image[Addr];
    Rdata = Rdata & (~(0xF0 >> ((X % 2) * 4))); // Clear first, then set value
    Paint.Image[Addr] = Rdata | ((Color << 4) >> ((X % 2) * 4));
    // printf("Add =  %d ,data = %d\r\n",Addr,Rdata);
  }
}

/******************************************************************************
function: Clear the color of the picture
parameter:
    Color : Painted colors
******************************************************************************/
void Paint_Clear(UWORD Color) {
  if (Paint.Scale == 2) {
    for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
      for (UWORD X = 0; X < Paint.WidthByte; X++) { // 8 pixel =  1 byte
        UDOUBLE Addr = X + Y * Paint.WidthByte;
        Paint.Image[Addr] = Color;
      }
    }
  } else if (Paint.Scale == 4) {
    for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
      for (UWORD X = 0; X < Paint.WidthByte; X++) {
        UDOUBLE Addr = X + Y * Paint.WidthByte;
        Paint.Image[Addr] = (Color << 6) | (Color << 4) | (Color << 2) | Color;
      }
    }
  } else if (Paint.Scale == 7 || Paint.Scale == 16) {
    for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
      for (UWORD X = 0; X < Paint.WidthByte; X++) {
        UDOUBLE Addr = X + Y * Paint.WidthByte;
        Paint.Image[Addr] = (Color << 4) | Color;
      }
    }
  }
}

/******************************************************************************
function: Clear the color of a window
parameter:
    Xstart : x starting point
    Ystart : Y starting point
    Xend   : x end point
    Yend   : y end point
    Color  : Painted colors
******************************************************************************/
void Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                        UWORD Color) {
  UWORD X, Y;
  for (Y = Ystart; Y < Yend; Y++) {
    for (X = Xstart; X < Xend; X++) { // 8 pixel =  1 byte
      Paint_SetPixel(X, Y, Color);
    }
  }
}

/******************************************************************************
function: Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		: The Xpoint coordinate of the point
    Ypoint		: The Ypoint coordinate of the point
    Color		: Painted color
    Dot_Pixel	: point size
    Dot_Style	: point Style
******************************************************************************/
void Paint_DrawPoint(UWORD Xpoint, UWORD Ypoint, UWORD Color,
                     DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_Style) {
  if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
    return;
  }

  int16_t XDir_Num, YDir_Num;
  if (Dot_Style == DOT_FILL_AROUND) {
    for (XDir_Num = 0; XDir_Num < 2 * Dot_Pixel - 1; XDir_Num++) {
      for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++) {
        if (Xpoint + XDir_Num - Dot_Pixel < 0 ||
            Ypoint + YDir_Num - Dot_Pixel < 0)
          break;
        // printf("x = %d, y = %d\r\n", Xpoint + XDir_Num - Dot_Pixel, Ypoint +
        // YDir_Num - Dot_Pixel);
        Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel,
                       Ypoint + YDir_Num - Dot_Pixel, Color);
      }
    }
  } else {
    for (XDir_Num = 0; XDir_Num < Dot_Pixel; XDir_Num++) {
      for (YDir_Num = 0; YDir_Num < Dot_Pixel; YDir_Num++) {
        Paint_SetPixel(Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
      }
    }
  }
}

/******************************************************************************
function: Draw a line of arbitrary slope
parameter:
    Xstart ：Starting Xpoint point coordinates
    Ystart ：Starting Xpoint point coordinates
    Xend   ：End point Xpoint coordinate
    Yend   ：End point Ypoint coordinate
    Color  ：The color of the line segment
    Line_width : Line width
    Line_Style: Solid and dotted lines
******************************************************************************/
void Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, DOT_PIXEL Line_width, LINE_STYLE Line_Style) {
  if (Xstart > Paint.Width || Ystart > Paint.Height || Xend > Paint.Width ||
      Yend > Paint.Height) {
    return;
  }

  UWORD Xpoint = Xstart;
  UWORD Ypoint = Ystart;
  int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
  int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

  // Increment direction, 1 is positive, -1 is counter;
  int XAddway = Xstart < Xend ? 1 : -1;
  int YAddway = Ystart < Yend ? 1 : -1;

  // Cumulative error
  int Esp = dx + dy;
  char Dotted_Len = 0;

  for (;;) {
    Dotted_Len++;
    // Painted dotted line, 2 point is really virtual
    if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
      Paint_DrawPoint(Xpoint, Ypoint, IMAGE_BACKGROUND, Line_width,
                      DOT_STYLE_DFT);
      Dotted_Len = 0;
    } else {
      Paint_DrawPoint(Xpoint, Ypoint, Color, Line_width, DOT_STYLE_DFT);
    }
    if (2 * Esp >= dy) {
      if (Xpoint == Xend)
        break;
      Esp += dy;
      Xpoint += XAddway;
    }
    if (2 * Esp <= dx) {
      if (Ypoint == Yend)
        break;
      Esp += dx;
      Ypoint += YAddway;
    }
  }
}

/******************************************************************************
function: Draw a rectangle
parameter:
    Xstart ：Rectangular  Starting Xpoint point coordinates
    Ystart ：Rectangular  Starting Xpoint point coordinates
    Xend   ：Rectangular  End point Xpoint coordinate
    Yend   ：Rectangular  End point Ypoint coordinate
    Color  ：The color of the Rectangular segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the rectangle
******************************************************************************/
void Paint_DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                         UWORD Color, DOT_PIXEL Line_width,
                         DRAW_FILL Draw_Fill) {
  if (Xstart > Paint.Width || Ystart > Paint.Height || Xend > Paint.Width ||
      Yend > Paint.Height) {
    return;
  }

  if (Draw_Fill) {
    UWORD Ypoint;
    for (Ypoint = Ystart; Ypoint < Yend; Ypoint++) {
      Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint, Color, Line_width,
                     LINE_STYLE_SOLID);
    }
  } else {
    Paint_DrawLine(Xstart, Ystart, Xend, Ystart, Color, Line_width,
                   LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart, Ystart, Xstart, Yend, Color, Line_width,
                   LINE_STYLE_SOLID);
    Paint_DrawLine(Xend, Yend, Xend, Ystart, Color, Line_width,
                   LINE_STYLE_SOLID);
    Paint_DrawLine(Xend, Yend, Xstart, Yend, Color, Line_width,
                   LINE_STYLE_SOLID);
  }
}

/******************************************************************************
function: Use the 8-point method to draw a circle of the
            specified size at the specified position->
parameter:
    X_Center  ：Center X coordinate
    Y_Center  ：Center Y coordinate
    Radius    ：circle Radius
    Color     ：The color of the ：circle segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the Circle
******************************************************************************/
void Paint_DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius, UWORD Color,
                      DOT_PIXEL Line_width, DRAW_FILL Draw_Fill) {
  if (X_Center > Paint.Width || Y_Center >= Paint.Height) {
    return;
  }

  // Draw a circle from(0, R) as a starting point
  int16_t XCurrent, YCurrent;
  XCurrent = 0;
  YCurrent = Radius;

  // Cumulative error,judge the next point of the logo
  int16_t Esp = 3 - (Radius << 1);

  int16_t sCountY;
  if (Draw_Fill == DRAW_FILL_FULL) {
    while (XCurrent <= YCurrent) { // Realistic circles
      for (sCountY = XCurrent; sCountY <= YCurrent; sCountY++) {
        Paint_DrawPoint(X_Center + XCurrent, Y_Center + sCountY, Color,
                        DOT_PIXEL_DFT, DOT_STYLE_DFT); // 1
        Paint_DrawPoint(X_Center - XCurrent, Y_Center + sCountY, Color,
                        DOT_PIXEL_DFT, DOT_STYLE_DFT); // 2
        Paint_DrawPoint(X_Center - sCountY, Y_Center + XCurrent, Color,
                        DOT_PIXEL_DFT, DOT_STYLE_DFT); // 3
        Paint_DrawPoint(X_Center - sCountY, Y_Center - XCurrent, Color,
                        DOT_PIXEL_DFT, DOT_STYLE_DFT); // 4
        Paint_DrawPoint(X_Center - XCurrent, Y_Center - sCountY, Color,
                        DOT_PIXEL_DFT, DOT_STYLE_DFT); // 5
        Paint_DrawPoint(X_Center + XCurrent, Y_Center - sCountY, Color,
                        DOT_PIXEL_DFT, DOT_STYLE_DFT); // 6
        Paint_DrawPoint(X_Center + sCountY, Y_Center - XCurrent, Color,
                        DOT_PIXEL_DFT, DOT_STYLE_DFT); // 7
        Paint_DrawPoint(X_Center + sCountY, Y_Center + XCurrent, Color,
                        DOT_PIXEL_DFT, DOT_STYLE_DFT);
      }
      if (Esp < 0)
        Esp += 4 * XCurrent + 6;
      else {
        Esp += 10 + 4 * (XCurrent - YCurrent);
        YCurrent--;
      }
      XCurrent++;
    }
  } else { // Draw a hollow circle
    while (XCurrent <= YCurrent) {
      Paint_DrawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color,
                      Line_width, DOT_STYLE_DFT); // 1
      Paint_DrawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color,
                      Line_width, DOT_STYLE_DFT); // 2
      Paint_DrawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color,
                      Line_width, DOT_STYLE_DFT); // 3
      Paint_DrawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color,
                      Line_width, DOT_STYLE_DFT); // 4
      Paint_DrawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color,
                      Line_width, DOT_STYLE_DFT); // 5
      Paint_DrawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color,
                      Line_width, DOT_STYLE_DFT); // 6
      Paint_DrawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color,
                      Line_width, DOT_STYLE_DFT); // 7
      Paint_DrawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color,
                      Line_width, DOT_STYLE_DFT); // 0

      if (Esp < 0)
        Esp += 4 * XCurrent + 6;
      else {
        Esp += 10 + 4 * (XCurrent - YCurrent);
        YCurrent--;
      }
      XCurrent++;
    }
  }
}

/******************************************************************************
function:	Display monochrome bitmap
parameter:
    image_buffer ：A picture data converted to a bitmap
info:
    Use a computer to convert the image into a corresponding array,
    and then embed the array directly into Imagedata.cpp as a .c file.
******************************************************************************/
void Paint_DrawBitMap(const unsigned char *image_buffer) {
  UWORD x, y;
  UDOUBLE Addr = 0;

  for (y = 0; y < Paint.HeightByte; y++) {
    for (x = 0; x < Paint.WidthByte; x++) { // 8 pixel =  1 byte
      Addr = x + y * Paint.WidthByte;
      Paint.Image[Addr] = (unsigned char)image_buffer[Addr];
    }
  }
}

/******************************************************************************
function:	Display image
parameter:
    image            ：Image start address
    xStart           : X starting coordinates
    yStart           : Y starting coordinates
    xEnd             ：Image width
    yEnd             : Image height
******************************************************************************/
void Paint_DrawImage(const unsigned char *image_buffer, UWORD xStart,
                     UWORD yStart, UWORD W_Image, UWORD H_Image) {
  UWORD x, y;
  UWORD w_byte = (W_Image % 8) ? (W_Image / 8) + 1 : W_Image / 8;
  UDOUBLE Addr = 0;
  UDOUBLE pAddr = 0;
  for (y = 0; y < H_Image; y++) {
    for (x = 0; x < w_byte; x++) { // 8 pixel =  1 byte
      Addr = x + y * w_byte;
      pAddr = x + (xStart / 8) + ((y + yStart) * Paint.WidthByte);
      Paint.Image[pAddr] = (unsigned char)image_buffer[Addr];
    }
  }
}
