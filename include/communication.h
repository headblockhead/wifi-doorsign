#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>

void DEV_SPI_WriteByte(uint8_t data);
uint8_t DEV_SPI_ReadByte();
void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t len);
