#include "communication.h"
#include "pinout.h"

void DEV_SPI_WriteByte(uint8_t data) {
  // SPI.beginTransaction(spi_settings);
  digitalWrite(PIN_SPI_CS, 0);

  for (int i = 0; i < 8; i++) {
    if ((data & 0x80) == 0)
      digitalWrite(PIN_SPI_DIN, 0);
    else
      digitalWrite(PIN_SPI_DIN, 1);

    data <<= 1;
    digitalWrite(PIN_SPI_SCK, 1);
    digitalWrite(PIN_SPI_SCK, 0);
  }

  // SPI.transfer(data);
  digitalWrite(PIN_SPI_CS, 1);
  // SPI.endTransaction();
}

uint8_t DEV_SPI_ReadByte() {
  uint8_t j = 0xff;
  pinMode(PIN_SPI_DIN, INPUT);
  digitalWrite(PIN_SPI_CS, 0);
  for (int i = 0; i < 8; i++) {
    j = j << 1;
    if (digitalRead(PIN_SPI_DIN))
      j = j | 0x01;
    else
      j = j & 0xfe;

    digitalWrite(PIN_SPI_SCK, 1);
    digitalWrite(PIN_SPI_SCK, 0);
  }
  digitalWrite(PIN_SPI_CS, 1);
  pinMode(PIN_SPI_DIN, OUTPUT);
  return j;
}

void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t len) {
  for (int i = 0; i < len; i++)
    DEV_SPI_WriteByte(pData[i]);
}
