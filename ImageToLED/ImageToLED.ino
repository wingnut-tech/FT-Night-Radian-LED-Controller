#include <FastLED.h>
#include "image.h"

#define NUM_LEDS_PER_WING 28
// #define NUM_LEDS_PER_WING 3
#define ROWS NUM_LEDS_PER_WING * 2
#define LEFT_PIN 11
#define RIGHT_PIN 12
// #define RIGHT_PIN 6
#define RC_PIN1 5
#define ON_TIME 10
#define OFF_TIME 15

#define TMP_BRIGHTNESS 32

unsigned long prevMillis = 0;

int totalPixels = sizeof(image);

CRGB leds[ROWS];


void setLED(int led, CRGB color) {
  // This part reverses the order of LEDs for one wing,
  // as they normally start at 0 in the inside, and I'm addressing both together
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

void myDelay(int delay) {
  // This just waits the "remainder" of the time until the next interval.
  // This is better than delay(), as the actual timing between tasks will be correct
  while (millis() - prevMillis < delay); // Just loop until it's time
  prevMillis = millis();
}

void blackout() {
  for (int i = 0; i < ROWS; i++) {
    leds[i] = CRGB::Black;
  }
}

void setup() {
  pinMode(RC_PIN1, INPUT);
  // Adding both outputs to the same leds[] array
  FastLED.addLeds<NEOPIXEL, LEFT_PIN>(leds, 0, NUM_LEDS_PER_WING); // From 0-(one wing)
  FastLED.addLeds<NEOPIXEL, RIGHT_PIN>(leds, NUM_LEDS_PER_WING, NUM_LEDS_PER_WING); // from (one wing)-end
}

void loop() {
  int direction = (pulseIn(RC_PIN1, HIGH, 25000) < 1500) ? 0 : 1;
  int count = 0;
  byte currentByte;
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      if (direction == 0) {
        currentByte = pgm_read_byte(&image[count]); // count is only updated every other time, because we use one byte for two pixels
      } else {
        currentByte = pgm_read_byte(&image[totalPixels - count]); // read the image[] array "backwards" to flip the image. Maybe?
      }
      if (y % 2 == 0) {
        setLED(y, colors[currentByte >> 4]); // first led gets the first half of the byte ( 0x(F)F ), shifted over
      } else {
        setLED(y, colors[currentByte & 0x0F]); // second led gets the second half of the byte ( 0xF(F) ) with the first half removed
        count++;
      }
    }
    myDelay(OFF_TIME);  // Delay the remainder of the "off time" between columns
    showStrip();        // Show the new "on" values that were queued
    blackout();         // Prepare the blackout for between columns
    myDelay(ON_TIME);   // Delay the remainder of the "on time" for the columns
    showStrip();        // Show the new "blacked out" values that were queued
  }
  // Between images, give some space
  blackout();
  showStrip();
  delay(3000);
}
