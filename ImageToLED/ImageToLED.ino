#include "image.h"
#include <FastLED.h>

#define ROWS 54
#define COLS 400

int leds[];

void loop () {
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y += 2) {
        byte currentByte = pgm_read_byte(&image[(x * ROWS) + y])
        leds[y] = colors[currentByte] << 4];
        leds[y+1] = colors[currentByte] & 0x0F];
    }
  }
}