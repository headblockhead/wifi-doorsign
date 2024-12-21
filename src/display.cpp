#include "display.h"
#include "communication.h"
#include "pinout.h"
#include <Arduino.h>
#include <stdint.h>

static void EPD_Reset(void) {
  digitalWrite(PIN_SPI_RST, 1);
  delay(200);
  digitalWrite(PIN_SPI_RST, 0);
  delay(1);
  digitalWrite(PIN_SPI_RST, 1);
  delay(200);
}

static void EPD_SendCommand(uint8_t Reg) {
  digitalWrite(PIN_SPI_DC, 0);
  digitalWrite(PIN_SPI_CS, 0);
  DEV_SPI_WriteByte(Reg);
  digitalWrite(PIN_SPI_CS, 1);
}

static void EPD_SendData(uint8_t Data) {
  digitalWrite(PIN_SPI_DC, 1);
  digitalWrite(PIN_SPI_CS, 0);
  DEV_SPI_WriteByte(Data);
  digitalWrite(PIN_SPI_CS, 1);
}

// Wait until the busy_pin goes HIGH
static void EPD_WaitUntilBusyHigh(void) {
  while (!(digitalRead(PIN_SPI_BUSY)))
    ;
}

// Wait until the busy_pin goes LOW
static void EPD_WaitUntilBusyLow(void) {
  while (digitalRead(PIN_SPI_BUSY))
    ;
}

void EPD_Init(void) {
  EPD_Reset();
  EPD_WaitUntilBusyHigh();
  EPD_SendCommand(0x00);
  EPD_SendData(0xEF);
  EPD_SendData(0x08);
  EPD_SendCommand(0x01);
  EPD_SendData(0x37);
  EPD_SendData(0x00);
  EPD_SendData(0x23);
  EPD_SendData(0x23);
  EPD_SendCommand(0x03);
  EPD_SendData(0x00);
  EPD_SendCommand(0x06);
  EPD_SendData(0xC7);
  EPD_SendData(0xC7);
  EPD_SendData(0x1D);
  EPD_SendCommand(0x30);
  EPD_SendData(0x3C);
  EPD_SendCommand(0x41);
  EPD_SendData(0x00);
  EPD_SendCommand(0x50);
  EPD_SendData(0x37);
  EPD_SendCommand(0x60);
  EPD_SendData(0x22);
  EPD_SendCommand(0x61);
  EPD_SendData(0x02);
  EPD_SendData(0x58);
  EPD_SendData(0x01);
  EPD_SendData(0xC0);
  EPD_SendCommand(0xE3);
  EPD_SendData(0xAA);

  delay(100);
  EPD_SendCommand(0x50);
  EPD_SendData(0x37);
}

void EPD_Clear(uint8_t color) {
  EPD_SendCommand(0x61);
  EPD_SendData(0x02);
  EPD_SendData(0x58);
  EPD_SendData(0x01);
  EPD_SendData(0xC0);
  EPD_SendCommand(0x10);
  for (uint32_t i = 0; i < DISPLAY_HEIGHT; i++) {
    for (uint32_t j = 0; j < DISPLAY_WIDTH / 2; j++)
      EPD_SendData((color << 4) | color);
  }
  EPD_SendCommand(0x04);
  EPD_WaitUntilBusyHigh();
  EPD_SendCommand(0x12);
  EPD_WaitUntilBusyHigh();
  EPD_SendCommand(0x02);
  EPD_WaitUntilBusyLow();
  delay(500);
}

void EPD_SendImage(const uint8_t *image) {
  EPD_SendCommand(0x61);
  EPD_SendData(0x02);
  EPD_SendData(0x58);
  EPD_SendData(0x01);
  EPD_SendData(0xC0);
  EPD_SendCommand(0x10);
  for (uint32_t i = 0; i < DISPLAY_HEIGHT; i++) {
    for (uint32_t j = 0; j < DISPLAY_WIDTH / 2; j++) {
      EPD_SendData(image[j + i * DISPLAY_WIDTH / 2]);
    }
  }
  EPD_SendCommand(0x04);
  EPD_WaitUntilBusyHigh();
  EPD_SendCommand(0x12);
  EPD_WaitUntilBusyHigh();
  EPD_SendCommand(0x02);
  EPD_WaitUntilBusyLow();
  delay(200);
}

void EPD_SendPartImage(const uint8_t *image, uint16_t xstart, uint16_t ystart,
                       uint16_t image_width, uint16_t image_heigh) {
  EPD_SendCommand(0x61);
  EPD_SendData(0x02);
  EPD_SendData(0x58);
  EPD_SendData(0x01);
  EPD_SendData(0xC0);
  EPD_SendCommand(0x10);
  for (uint32_t i = 0; i < DISPLAY_HEIGHT; i++) {
    for (uint32_t j = 0; j < DISPLAY_WIDTH / 2; j++) {
      if (i < image_heigh + ystart && i >= ystart &&
          j < (image_width + xstart) / 2 && j >= xstart / 2) {
        EPD_SendData(
            image[(j - xstart / 2) + (image_width / 2 * (i - ystart))]);
      } else {
        EPD_SendData(0x11);
      }
    }
  }
  EPD_SendCommand(0x04);
  EPD_WaitUntilBusyHigh();
  EPD_SendCommand(0x12);
  EPD_WaitUntilBusyHigh();
  EPD_SendCommand(0x02);
  EPD_WaitUntilBusyLow();
  delay(200);
}

void EPD_Sleep(void) {
  delay(100);
  EPD_SendCommand(0x07);
  EPD_SendData(0xA5);
  delay(100);
  digitalWrite(PIN_SPI_RST, 0);
}
