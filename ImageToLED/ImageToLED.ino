#include "image.h"
#include <FastLED.h>

#define NUM_LEDS_PER_WING 27
#define ROWS NUM_LEDS_PER_WING * 2
#define COLS 400
#define LEFT_PIN 11
#define RIGHT_PIN 12

// CRGB rightleds[NUM_LEDS_PER_WING];
// CRGB leftleds[NUM_LEDS_PER_WING];
CRGB leds[ROWS];

void setup() {
  FastLED.addLeds<NEOPIXEL, RIGHT_PIN>(leds, 0, NUM_LEDS_PER_WING);
  FastLED.addLeds<NEOPIXEL, LEFT_PIN>(leds, NUM_LEDS_PER_WING, NUM_LEDS_PER_WING);
}

void setLED(int led, int color) {
  if (led <= NUM_LEDS_PER_WING) {
    led = NUM_LEDS_PER_WING - led;
  }
  leds[led] = color;
}

void loop() {
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y += 2) {
        byte currentByte = pgm_read_byte(&image[(x * ROWS) + y])
        // leds[y] = colors[currentByte] << 4];
        // leds[y+1] = colors[currentByte] & 0x0F];
        setLED(y, colors[currentByte] << 4]);
        setLED(y+1, colors[currentByte] & 0x0F]);
    }
  }
}