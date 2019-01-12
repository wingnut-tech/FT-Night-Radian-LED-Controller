#include <FastLED.h>
#include "image.h"

// #define NUM_LEDS_PER_WING 28
#define NUM_LEDS_PER_WING 3
#define ROWS NUM_LEDS_PER_WING * 2
#define LEFT_PIN 11
// #define RIGHT_PIN 12
#define RIGHT_PIN 6
#define DELAY 100

#define TMP_BRIGHTNESS 32

CRGB leds[ROWS];


void setup() {
  FastLED.addLeds<NEOPIXEL, RIGHT_PIN>(leds, 0, NUM_LEDS_PER_WING);
  FastLED.addLeds<NEOPIXEL, LEFT_PIN>(leds, NUM_LEDS_PER_WING, NUM_LEDS_PER_WING);
}

void setLED(int led, CRGB color) {
  if (led < NUM_LEDS_PER_WING) {
    led = NUM_LEDS_PER_WING - 1 - led;
  }
  leds[led] = color;
}

void showStrip () {
  #ifdef TMP_BRIGHTNESS
  FastLED.setBrightness(TMP_BRIGHTNESS);
  #endif
  FastLED.show();
}

void loop() {
  int count = 0;
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
        byte currentByte = pgm_read_byte(&image[count]);
        if (y % 2 == 0) {
          setLED(y, colors[currentByte >> 4]);
        } else {
          setLED(y, colors[currentByte & 0x0F]);
          count++;
        }
    }
    showStrip();
    delay(DELAY);
  }
  for (int i = 0; i < ROWS; i++) {
    leds[i] = CRGB::Black;
  }
  showStrip();
  Serial.println();
  delay(3000);
}
