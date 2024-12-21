#pragma once

/* Max size of bitmap will based on a font24 (17x24) */
#define MAX_HEIGHT_FONT 41
#define MAX_WIDTH_FONT 32
#define OFFSET_BITMAP 54

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct _tFont {
  const uint8_t *table;
  uint16_t Width;
  uint16_t Height;
} FONT;

extern FONT Font24;
extern FONT Font20;
extern FONT Font16;
extern FONT Font12;
extern FONT Font8;

#ifdef __cplusplus
}
#endif
