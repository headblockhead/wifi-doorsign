#include "epaper_dev.h"

void GPIO_Config(void) {
  pinMode(EPD_BUSY_PIN, INPUT);
  pinMode(EPD_RST_PIN, OUTPUT);
  pinMode(EPD_DC_PIN, OUTPUT);

  pinMode(EPD_SCK_PIN, OUTPUT);
  pinMode(EPD_MOSI_PIN, OUTPUT);
  pinMode(EPD_CS_PIN, OUTPUT);

  digitalWrite(EPD_CS_PIN, HIGH);
  digitalWrite(EPD_SCK_PIN, LOW);
}

void GPIO_Mode(UWORD GPIO_Pin, UWORD Mode) {
  if (Mode == 0) {
    pinMode(GPIO_Pin, INPUT);
  } else {
    pinMode(GPIO_Pin, OUTPUT);
  }
}

UBYTE DEV_Module_Init(void) {
  GPIO_Config();

  // spi
  // SPI.setDataMode(SPI_MODE0);
  // SPI.setBitOrder(MSBFIRST);
  // SPI.setClockDivider(SPI_CLOCK_DIV4);
  // SPI.begin();

  return 0;
}

void DEV_SPI_WriteByte(UBYTE data) {
  // SPI.beginTransaction(spi_settings);
  digitalWrite(EPD_CS_PIN, GPIO_PIN_RESET);

  for (int i = 0; i < 8; i++) {
    if ((data & 0x80) == 0)
      digitalWrite(EPD_MOSI_PIN, GPIO_PIN_RESET);
    else
      digitalWrite(EPD_MOSI_PIN, GPIO_PIN_SET);

    data <<= 1;
    digitalWrite(EPD_SCK_PIN, GPIO_PIN_SET);
    digitalWrite(EPD_SCK_PIN, GPIO_PIN_RESET);
  }

  // SPI.transfer(data);
  digitalWrite(EPD_CS_PIN, GPIO_PIN_SET);
  // SPI.endTransaction();
}

UBYTE DEV_SPI_ReadByte() {
  UBYTE j = 0xff;
  GPIO_Mode(EPD_MOSI_PIN, 0);
  digitalWrite(EPD_CS_PIN, GPIO_PIN_RESET);
  for (int i = 0; i < 8; i++) {
    j = j << 1;
    if (digitalRead(EPD_MOSI_PIN))
      j = j | 0x01;
    else
      j = j & 0xfe;

    digitalWrite(EPD_SCK_PIN, GPIO_PIN_SET);
    digitalWrite(EPD_SCK_PIN, GPIO_PIN_RESET);
  }
  digitalWrite(EPD_CS_PIN, GPIO_PIN_SET);
  GPIO_Mode(EPD_MOSI_PIN, 1);
  return j;
}

void DEV_SPI_Write_nByte(UBYTE *pData, UDOUBLE len) {
  for (int i = 0; i < len; i++)
    DEV_SPI_WriteByte(pData[i]);
}
