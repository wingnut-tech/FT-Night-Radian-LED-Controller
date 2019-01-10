#include <FastLED.h>
// #include <BMP280.h>
#include <Adafruit_BMP280.h>
// #define P0 1021.97
// define number of LEDs in specific strings
#define WING_LEDS 31
#define NON_NAV_LEDS 20
#define FUSE_LEDS 18
#define NOSE_LEDS 4
#define TAIL_LEDS 5
#define MIN_BRIGHTNESS 32
#define MAX_BRIGHTNESS 255

// define the pins that the LED strings are connected to
#define TAIL_PIN 8
#define FUSE_PIN 9
#define NOSE_PIN 10
#define LEFT_PIN 11
#define RIGHT_PIN 12

#define RC_PIN1 5   // Pin 5 Connected to Receiver;
#define NUM_SHOWS 9
#define TMP_BRIGHTNESS 255
int currentCh1 = 0;  // Receiver Channel PPM value
int prevCh1 = 0; // determine if the Receiver signal changed

CRGB rightleds[WING_LEDS];
CRGB leftleds[WING_LEDS];
CRGB noseleds[NOSE_LEDS];
CRGB fuseleds[FUSE_LEDS];
CRGB tailleds[TAIL_LEDS];

int currentShow = 0; // which LED show are we currently running
int prevShow = 0; // did the LED show change
unsigned long prevMillis = 0;

int interval;
// BMP280 bmp;
Adafruit_BMP280 bmp;
float relativeAlt;

// Static patterns
char usflag[] = {'b', 'b', 'b', 'w', 'w', 'w', 'w', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'w', 'w', 'w', 'r', 'r', 'r', 'b', 'b', 'b'};
char init_rightwing[] = {'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g'};
char rightwing[] = {'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g'};
char init_leftwing[] = {'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r'};
char leftwing[] = {'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r'};
char init_nose[] = {'b', 'b', 'b', 'b'};
char init_fuse[] = {'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b'};
char init_tail[] = {'b', 'b', 'b', 'b', 'b'};
char christmas[] = {'r', 'r', 'r', 'g', 'g', 'g', 'g', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'g', 'g', 'g', 'r', 'r', 'r', 'g', 'g', 'g'};

// Gradients
DEFINE_GRADIENT_PALETTE( orange_yellow ) {
  0,   255,165,  0,   //orange
128,   255,244,175,   //bright white-yellow
255,   255,246,  0 }; //

DEFINE_GRADIENT_PALETTE( teal_blue ) {
  0,     0,244,216,   //tealish
255,    48,130,219 }; //blueish

DEFINE_GRADIENT_PALETTE( pure_white ) {
  0,    255,255,255,
255,    255,255,255 };

DEFINE_GRADIENT_PALETTE( warm_white ) {
  0,    255,172, 68,
255,    255,172, 68 };

DEFINE_GRADIENT_PALETTE( blue ) {
  0,    0,0,255,
255,    0,0,255 };

DEFINE_GRADIENT_PALETTE( blue_black ) {
  0,    0,0,0,
255,    0,0,255 };

DEFINE_GRADIENT_PALETTE( variometer ) {
  0,    255,0,0, //Red
128,    255,255,255, //White
255,    0,255,0 }; //Green

void setup() {
  bmp.begin(); // initialize the altitude pressure sensor
  // bmp.setOversampling(4);
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  
  relativeAlt = bmp.readPressure()/100;

  //Serial.begin(9600);
  pinMode(RC_PIN1, INPUT);
  FastLED.addLeds<NEOPIXEL, RIGHT_PIN>(rightleds, WING_LEDS);
  FastLED.addLeds<NEOPIXEL, LEFT_PIN>(leftleds, WING_LEDS);
  FastLED.addLeds<NEOPIXEL, FUSE_PIN>(fuseleds, FUSE_LEDS);
  FastLED.addLeds<NEOPIXEL, NOSE_PIN>(noseleds, NOSE_LEDS);
  FastLED.addLeds<NEOPIXEL, TAIL_PIN>(tailleds, TAIL_LEDS);
}

void loop() {
  static bool firstrun = true;
  static int prevModeIn = 0;
  static int currentModeIn = 0;

  if (firstrun) {
    setInitPattern(); // Set the LED strings to their boot-up configuration
    firstrun = false;
  }

  // Read in the length of the signal in microseconds
  prevCh1 = currentCh1;
  currentCh1 = pulseIn(RC_PIN1, HIGH, 25000);  // (Pin, State, Timeout)
  //currentCh1 = 1500;
  if (currentCh1 < 700) {currentCh1 = prevCh1;} // if signal is lost or poor quality, we continue running the same show

  currentModeIn = round(currentCh1/100);
  if (currentModeIn != prevModeIn) {
    currentShow = map(currentModeIn, 9, 19, 0, NUM_SHOWS-1); // mapping 9-19 to get the 900ms - 1900ms value
/*        Serial.write(27);       // ESC command
    Serial.print("[2J");    // clear screen command
    Serial.write(27);
    Serial.print("[H");     // cursor to home command
    Serial.print("Current Mode: ");
    Serial.println(currentShow);
    Serial.print("Channel value: ");
    Serial.println(currentCh1);
    Serial.print("Brightness value: ");
    Serial.println("------------");
    //delay(200);*/

    prevModeIn = currentModeIn;
  }

  // The timing control for calling each "frame" of the different animations
  unsigned long currentMillis = millis();
  if (currentMillis - prevMillis > interval) {
    prevMillis = currentMillis;
    stepShow();
  }
}

void stepShow() { // the main menu of different shows
  switch (currentShow) {
    case 0: blank(); //all off
            break;
    case 1: colorWave1(10);//regular rainbow
            break;
    case 2: twinkle1();
            break;
    case 3: setColor(blue);
            break;
    case 4: setColor(pure_white);
            break;
    case 5: chase();
            break;
    case 6: strobe(3);
            break;
    case 7: strobe(2);
            break;
    case 8: strobe(1);
            break;
  }
  prevShow = currentShow;
}

// Helper functions
void showStrip () {
  #ifdef TMP_BRIGHTNESS
  FastLED.setBrightness(TMP_BRIGHTNESS);
  #endif
  FastLED.show();
}

void blank() { // Turn off all LEDs
  for (int i = 0; i < NON_NAV_LEDS; i++) {
    rightleds[i] = CRGB::Black;
    leftleds[i] = CRGB::Black;
  }
  for (int i = 0; i < NOSE_LEDS; i++) {noseleds[i] = CRGB::Black;}
  for (int i = 0; i < FUSE_LEDS; i++) {fuseleds[i] = CRGB::Black;}
  for (int i = 0; i < TAIL_LEDS; i++) {tailleds[i] = CRGB::White;}
  showStrip();
}

void setColor (CRGBPalette16 palette) {
  for (int i; i < NON_NAV_LEDS; i++) {
    rightleds[i] = ColorFromPalette(palette, map(i, 0, NON_NAV_LEDS, 0, 240));
    leftleds[i] = ColorFromPalette(palette, map(i, 0, NON_NAV_LEDS, 0, 240));
    if (i < NOSE_LEDS) {noseleds[i] = ColorFromPalette(palette, map(i, 0, NOSE_LEDS, 0, 240));}
    if (i < FUSE_LEDS) {fuseleds[i] = ColorFromPalette(palette, map(i, 0, FUSE_LEDS, 0, 240));}
    if (i < TAIL_LEDS) {tailleds[i] = ColorFromPalette(palette, map(i, 0, TAIL_LEDS, 0, 240));}
  }
  interval = 20;
  showStrip();
}

CRGB LetterToColor (char letter) { // Convert the letters in the static patterns to color values
  CRGB color;
  switch (letter) {
    case 'r': color = CRGB::Red;
              break;
    case 'g': color = CRGB::Green;
              break;
    case 'b': color = CRGB::Blue;
              break;
    case 'w': color = CRGB::White;
              break;
    case 'a': color = CRGB::AntiqueWhite;
              break;
    case 'o': color = CRGB::Black;
              break;
  }
  return color;
}

//TODO: test and make sure this new LetterToColor function actually works.
//      If it does, we can nuke all of this redundant commented code.
void setPattern (char pattern[]) {
  for (int i = 0; i < NON_NAV_LEDS; i++) {
    // CRGB newColor;
    // switch (pattern[i]) {
    //   case 'r': newColor = CRGB::Red;
    //             break;
    //   case 'g': newColor = CRGB::Green;
    //             break;
    //   case 'b': newColor = CRGB::Blue;
    //             break;
    //   case 'w': newColor = CRGB::White;
    //             break;
    //   case 'a': newColor = CRGB::AntiqueWhite;
    //             break;
    //   case 'o': newColor = CRGB::Black;
    //             break;
    // }
    // rightleds[i] = newColor;
    // leftleds[i] = newColor;
    rightleds[i] = LetterToColor(pattern[i]);
    leftleds[i] = LetterToColor(pattern[i]);
  }
  interval = 20;
  showStrip();
}

void setInitPattern () {

  for (int i = 0; i < WING_LEDS; i++) {
    // CRGB newColor;
    // switch (init_rightwing[i]) {
    //   case 'r': newColor = CRGB::Red;
    //             break;
    //   case 'g': newColor = CRGB::Green;
    //             break;
    //   case 'b': newColor = CRGB::Blue;
    //             break;
    //   case 'w': newColor = CRGB::White;
    //             break;
    //   case 'a': newColor = CRGB::AntiqueWhite;
    //             break;
    //   case 'o': newColor = CRGB::Black;
    //             break;
    // }
    // rightleds[i] = newColor;
    rightleds[i] = LetterToColor(init_rightwing[i]);
  }
  
  for (int i = 0; i < WING_LEDS; i++) {
    // CRGB newColor;
    // switch (init_leftwing[i]) {
    //   case 'r': newColor = CRGB::Red;
    //             break;
    //   case 'g': newColor = CRGB::Green;
    //             break;
    //   case 'b': newColor = CRGB::Blue;
    //             break;
    //   case 'w': newColor = CRGB::White;
    //             break;
    //   case 'a': newColor = CRGB::AntiqueWhite;
    //             break;
    //   case 'o': newColor = CRGB::Black;
    //             break;
    // }
    // leftleds[i] = newColor;
    leftleds[i] = LetterToColor(init_leftwing[i]);
  }
  
  for (int i = 0; i < NOSE_LEDS; i++) {
    // CRGB newColor;
    // switch (init_nose[i]) {
    //   case 'r': newColor = CRGB::Red;
    //             break;
    //   case 'g': newColor = CRGB::Green;
    //             break;
    //   case 'b': newColor = CRGB::Blue;
    //             break;
    //   case 'w': newColor = CRGB::White;
    //             break;
    //   case 'a': newColor = CRGB::AntiqueWhite;
    //             break;
    //   case 'o': newColor = CRGB::Black;
    //             break;
    // }
    // noseleds[i] = newColor;
    noseleds[i] = LetterToColor(init_nose[i]);
  }
  
  for (int i = 0; i < FUSE_LEDS; i++) {
    // CRGB newColor;
    // switch (init_fuse[i]) {
    //   case 'r': newColor = CRGB::Red;
    //             break;
    //   case 'g': newColor = CRGB::Green;
    //             break;
    //   case 'b': newColor = CRGB::Blue;
    //             break;
    //   case 'w': newColor = CRGB::White;
    //             break;
    //   case 'a': newColor = CRGB::AntiqueWhite;
    //             break;
    //   case 'o': newColor = CRGB::Black;
    //             break;
    // }
    // fuseleds[i] = newColor;
    fuseleds[i] = LetterToColor(init_fuse[i]);
  }
  
  for (int i = 0; i < TAIL_LEDS; i++) {
    // CRGB newColor;
    // switch (init_tail[i]) {
    //   case 'r': newColor = CRGB::Red;
    //             break;
    //   case 'g': newColor = CRGB::Green;
    //             break;
    //   case 'b': newColor = CRGB::Blue;
    //             break;
    //   case 'w': newColor = CRGB::White;
    //             break;
    //   case 'a': newColor = CRGB::AntiqueWhite;
    //             break;
    //   case 'o': newColor = CRGB::Black;
    //             break;
    // }
    // tailleds[i] = newColor;
    tailleds[i] = LetterToColor(init_tail[i]);
  }
  
  showStrip();
}

void animateColor (CRGBPalette16 palette, int ledOffset, int stepSize) {
  static int currentStep = 0;

  if (currentStep > 255) {currentStep = 0;}
  for (int i = 0; i < NON_NAV_LEDS; i++) {
      int j = triwave8((i * ledOffset) + currentStep);
      rightleds[i] = ColorFromPalette(palette, scale8(j, 240));
      leftleds[i] = ColorFromPalette(palette, scale8(j, 240));
  }

  currentStep += stepSize;
  interval = 20;
  showStrip();
}
// End helper functions


// Animations

void colorWave1 (int ledOffset) {
  if (prevShow != currentShow) {blank();}
  static int currentStep = 0;
  if (currentStep > 255) {currentStep = 0;}
  for (int j = 0; j < NON_NAV_LEDS; j++) {
    rightleds[j] = CHSV(currentStep + (ledOffset * j), 255, 255);
    leftleds[j] = CHSV(currentStep + (ledOffset * j), 255, 255);
    if (j < FUSE_LEDS) {fuseleds[j] = CHSV(currentStep + (ledOffset * j), 255, 255);}
  }
  currentStep++;
  interval = 10;
  showStrip();
}

void chase() {
  static int chaseStep = 0;
  if (prevShow != currentShow) {blank();} // blank all LEDs at the start of this show
  
  if (chaseStep > NON_NAV_LEDS) {
    rightleds[NON_NAV_LEDS] = CRGB::Black;
    leftleds[NON_NAV_LEDS] = CRGB::Black;
    chaseStep = 0;
  }

  rightleds[chaseStep] = CRGB::White;
  leftleds[chaseStep] = CRGB::White;
  if (chaseStep < NOSE_LEDS) {noseleds[chaseStep] = CRGB::White;}
  if (chaseStep < FUSE_LEDS) {fuseleds[chaseStep] = CRGB::White;}
  if (chaseStep < TAIL_LEDS) {tailleds[chaseStep] = CRGB::White;}

  rightleds[chaseStep-1] = CRGB::Black;
  leftleds[chaseStep-1] = CRGB::Black;
  if (chaseStep < NOSE_LEDS+1) {noseleds[chaseStep-1] = CRGB::Black;}
  if (chaseStep < FUSE_LEDS+1) {fuseleds[chaseStep-1] = CRGB::Black;}
  if (chaseStep < TAIL_LEDS+1) {tailleds[chaseStep-1] = CRGB::Black;}

  showStrip();
  chaseStep++;
  interval = 30;
}


void strobe(int style) {
  static bool StrobeState = true;
  if (prevShow != currentShow) {blank();}

  switch(style) {

    case 1: //Rapid strobing all LEDS in unison
      if (StrobeState) {
        for (int i = 0; i < NON_NAV_LEDS; i++) {
          rightleds[i] = CRGB::White;
          leftleds[i] = CRGB::White;
        }
        for (int i = 0; i < NOSE_LEDS; i++) {noseleds[i] = CRGB::White;}
        for (int i = 0; i < FUSE_LEDS; i++) {fuseleds[i] = CRGB::White;}
        for (int i = 0; i < TAIL_LEDS; i++) {tailleds[i] = CRGB::White;}
        StrobeState = false;
      } else {
        for (int i = 0; i < NON_NAV_LEDS; i++) {
          rightleds[i] = CRGB::Black;
          leftleds[i] = CRGB::Black;
        }
        for (int i = 0; i < NOSE_LEDS; i++) {noseleds[i] = CRGB::Black;}
        for (int i = 0; i < FUSE_LEDS; i++) {fuseleds[i] = CRGB::Black;}
        for (int i = 0; i < TAIL_LEDS; i++) {tailleds[i] = CRGB::Black;}
        StrobeState = true;
        }
      interval = 50;
      showStrip();
    break;

    case 2: //Alternate strobing of left and right wing
      if (StrobeState) {
        for (int i = 0; i < NON_NAV_LEDS; i++) {
          rightleds[i] = CRGB::White;
          leftleds[i] = CRGB::Black;
        }
        for (int i = 0; i < NOSE_LEDS; i++) {noseleds[i] = CRGB::Blue;}
        for (int i = 0; i < FUSE_LEDS; i++) {fuseleds[i] = CRGB::Blue;}
        for (int i = 0; i < TAIL_LEDS; i++) {tailleds[i] = CRGB::White;}
      } else {
        for (int i = 0; i < NON_NAV_LEDS; i++) {
          rightleds[i] = CRGB::Black;
          leftleds[i] = CRGB::White;
        }
        for (int i = 0; i < NOSE_LEDS; i++) {noseleds[i] = CRGB::Yellow;}
        for (int i = 0; i < FUSE_LEDS; i++) {fuseleds[i] = CRGB::Yellow;}
        for (int i = 0; i < TAIL_LEDS; i++) {tailleds[i] = CRGB::White;}
      }
      interval = 500;
      StrobeState = !StrobeState;
      showStrip();
    break;

    case 3: //alternate double-blink strobing of left and right wing
      static int strobeStep = 0;
      // CRGB noseColor = CRGB::Blue;
      // CRGB fuseColor = CRGB::Blue;
      // CRGB tailColor = CRGB::Blue;
      //Not sure what these were ^ ?

      switch(strobeStep) {

        case 0: // Right wing on for 50ms
          for (int i = 0; i < NON_NAV_LEDS; i++) {
            rightleds[i] = CRGB::White;
            leftleds[i] = CRGB::Black;
          }
          interval = 50;
        break;
          
        case 1: // Both wings off for 50ms
          for (int i = 0; i < NON_NAV_LEDS; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::Black;
          }
          interval = 50;
        break;
          
        case 2: // Right wing on for 50ms
          for (int i = 0; i < NON_NAV_LEDS; i++) {
            rightleds[i] = CRGB::White;
            leftleds[i] = CRGB::Black;
          }
          interval = 50;
        break;
          
        case 3: // Both wings off for 500ms
          for (int i = 0; i < NON_NAV_LEDS; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::Black;
          }
          interval = 500;
        break;
          
        case 4: // Left wing on for 50ms
          for (int i = 0; i < NON_NAV_LEDS; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::White;
          }
          interval = 50;
        break;
          
        case 5: // Both wings off for 50ms
          for (int i = 0; i < NON_NAV_LEDS; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::Black;
          }
          interval = 50;
        break;
          
        case 6: // Left wing on for 50ms
          for (int i = 0; i < NON_NAV_LEDS; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::White;
          }
          interval = 50;
        break;
          
        case 7: // Both wings off for 500ms
          for (int i = 0; i < NON_NAV_LEDS; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::Black;
          }
          interval = 500;
        break;
                    
      }

      showStrip();
      strobeStep++;
      if (strobeStep == 8) {strobeStep = 0;}
    break;

  }
}

void altitude() {
  static int majorAlt;
  static int minorAlt;
  static float prevAlt;
  // double T, P;
  // char result = bmp.startMeasurment();

  // if (result != 0) {
  //   delay(result);
  //   result = bmp.getTemperatureAndPressure(T, P);

  //   if (result != 0) {
  //     double currentAlt = bmp.altitude(P, P0);

  float currentAlt = bmp.readAltitude(relativeAlt)*3.28084;
  //using pressure when powered on, gives relative altitude from ground level. Also convert to feet.

  majorAlt = floor(currentAlt/100);
  minorAlt = int(currentAlt) % 100;
  for (int i=0; i < majorAlt; i++) {
    rightleds[i] = CRGB::White;
    rightleds[i+1] = CRGB::White;
    rightleds[i+2] = CRGB::White;
    leftleds[i] = CRGB::White;
    leftleds[i+1] = CRGB::White;
    leftleds[i+2] = CRGB::White;
  }
  for (int i=majorAlt+1; i < FUSE_LEDS; i++) {
    rightleds[i] = CRGB::Black;
    rightleds[i+1] = CRGB::Black;
    rightleds[i+2] = CRGB::Black;
    leftleds[i] = CRGB::Black;
    leftleds[i+1] = CRGB::Black;
    leftleds[i+2] = CRGB::Black;
  }
  //map vertical speed value to gradient pallet
  for (int i; i < TAIL_LEDS; i++) {
    tailleds[i] = ColorFromPalette(variometer, map(currentAlt-prevAlt, -10, 10, 0, 240));
  }
  prevAlt = currentAlt;
  //   }
  // }
  interval = 250;
  showStrip();
}


//TODO: twinkle() is not gonna work in the current setup.
//      Needs to be re-written to handle all leds together,
//      versus treating each section (nose, fuse, wings, etc) individually.
enum {SteadyDim, Dimming, Brightening};
void twinkle1 () {
  static int pixelState[NON_NAV_LEDS];
  const CRGB colorDown = CRGB(1, 1, 1);
  const CRGB colorUp = CRGB(8, 8, 8);
  const CRGB colorMax = CRGB(128, 128, 128);
  const CRGB colorMin = CRGB(4, 4, 4);
  const int twinkleChance = 1;

  if (prevShow != currentShow) {
    memset(pixelState, SteadyDim, sizeof(pixelState));
    for (int i = 0; i < NON_NAV_LEDS; i++) {
      rightleds[i] = colorMin;
      leftleds[i] = colorMin;
      if (i < NOSE_LEDS) {noseleds[i] = colorMin;}
      if (i < FUSE_LEDS) {fuseleds[i] = colorMin;}
      if (i < TAIL_LEDS) {tailleds[i] = colorMin;}
    }
  }

  for (int i = 0; i < NON_NAV_LEDS; i++) {
    if (pixelState[i] == SteadyDim) {
      if (random8() < twinkleChance) {
        pixelState[i] = Brightening;
      }
    }

    if (pixelState[i] == Brightening) {
      if (rightleds[i] >= colorMax) {
        pixelState[i] = Dimming;
      } else {
        rightleds[i] += colorUp;
        leftleds[i] += colorUp;
        if (i < NOSE_LEDS) {noseleds[i] += colorUp;}
        if (i < FUSE_LEDS) {fuseleds[i] += colorUp;}
        if (i < TAIL_LEDS) {tailleds[i] += colorUp;}

      }
    }
    if (pixelState[i] == Dimming) {
      if (rightleds[i] <= colorMin) {
        rightleds[i] = colorMin;
        leftleds[i] = colorMin;
        noseleds[i] = colorMin;
        fuseleds[i] = colorMin;
        tailleds[i] = colorMin;
        pixelState[i] = SteadyDim;
      } else {
        rightleds[i] -= colorDown;
        leftleds[i] -= colorDown;
        if (i < NOSE_LEDS) {noseleds[i] += colorDown;}
        if (i < FUSE_LEDS) {fuseleds[i] += colorDown;}
        if (i < TAIL_LEDS) {tailleds[i] += colorDown;}
      }
    }
  }
  interval = 10;
  showStrip();
}
