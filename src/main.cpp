#include "display.h"
#include "paint.h"
#include "pinout.h"
#include <Arduino.h>

unsigned char image1[134400];

void setup() {
  Serial.begin(115200);
  printf("Starting...\n");

  printf("Setting up SPI\n");
  pinMode(PIN_SPI_BUSY, INPUT);
  pinMode(PIN_SPI_RST, OUTPUT);
  pinMode(PIN_SPI_DC, OUTPUT);
  pinMode(PIN_SPI_SCK, OUTPUT);
  digitalWrite(PIN_SPI_SCK, 0);
  pinMode(PIN_SPI_DIN, OUTPUT);
  pinMode(PIN_SPI_CS, OUTPUT);
  digitalWrite(PIN_SPI_CS, 1);

  printf("Initializing EPD\n");
  EPD_Init();

  // printf("Clearing EPD\n");
  // EPD_Clear(COLOR_WHITE);

  printf("Drawing...\n");
  Paint_NewImage(image1, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, EPD_COLOR_WHITE);
  Paint_Clear(EPD_COLOR_WHITE);
  Paint_DrawRectangle(10, 10, 110, 110, EPD_COLOR_BLUE, DOT_PIXEL_1X1,
                      DRAW_FILL_FULL);
  Paint_DrawRectangle(120, 10, 220, 110, EPD_COLOR_BLACK, DOT_PIXEL_1X1,
                      DRAW_FILL_FULL);

  printf("Sending to EPD\n");
  EPD_SendImage(image1);

  printf("Sleeping\n");
  EPD_Sleep();
}

void loop() {}
