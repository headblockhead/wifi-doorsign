#include "epaper.h"
#include "epaper_dev.h"
#include "image.h"
#include "paint.h"

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>

void setup(void) {
  printf("Hello world!\n");

  DEV_Module_Init();
  EPD_5IN65F_Init();
  EPD_5IN65F_Clear(EPD_5IN65F_WHITE);
  DEV_Delay_ms(100);

  UBYTE *blackImage;
  UDOUBLE Imagesize =
      ((EPD_5IN65F_WIDTH % 2 == 0) ? (EPD_5IN65F_WIDTH / 2)
                                   : (EPD_5IN65F_WIDTH / 2 + 1)) *
      EPD_5IN65F_HEIGHT;
  printf("Imagesize %d\r\n", Imagesize);
  if ((blackImage = (UBYTE *)malloc(Imagesize / 2)) == NULL) {
    printf("Failed to apply for black memory...\r\n");
    while (1)
      ;
  }
  Paint_NewImage(blackImage, EPD_5IN65F_WIDTH, EPD_5IN65F_HEIGHT / 2, 0,
                 EPD_5IN65F_WHITE);
  Paint_SetScale(7);

  EPD_5IN65F_Display(gImage_5in65f_test);
  EPD_5IN65F_Sleep();

  free(blackImage);
  blackImage = NULL;

  DEV_Delay_ms(5000);
}

void loop(void) {}
