#include <FastLED.h>
#include <BMP280.h>
#define P0 1021.97
#define WING_LEDS 31
#define NON_NAV_LEDS 20
#define FUSE_LEDS 18
#define NOSE_LEDS 4
#define TAIL_LEDS 5
#define MIN_BRIGHTNESS 32
#define MAX_BRIGHTNESS 255
#define TAIL_PIN 8
#define FUSE_PIN 9
#define NOSE_PIN 10
#define LEFT_PIN 11
#define RIGHT_PIN 12
#define rcPin1 5   // Pin 5 Connected to Receiver;
#define NUM_SHOWS 9
#define TMP_BRIGHTNESS 255
int ch1 = 0;  // Receiver Channel PPM value
int prevch1 = 0;
CRGB rightleds[WING_LEDS];
CRGB leftleds[WING_LEDS];
CRGB noseleds[NOSE_LEDS];
CRGB fuseleds[FUSE_LEDS];
CRGB tailleds[TAIL_LEDS];
int currentShow = 0;
int prevShow = 0;
// int knobRemainder = 0;
unsigned long prevMillis = 0;
bool oldButton = HIGH;
int interval;
static bool firstrun = true;

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

void setup() {
  bmp.begin();
  bmp.setOversampling(4);
  //Serial.begin(9600);
  pinMode(rcPin1, INPUT);
  FastLED.addLeds<NEOPIXEL, RIGHT_PIN>(rightleds, WING_LEDS);
  FastLED.addLeds<NEOPIXEL, LEFT_PIN>(leftleds, WING_LEDS);
  FastLED.addLeds<NEOPIXEL, FUSE_PIN>(fuseleds, FUSE_LEDS);
  FastLED.addLeds<NEOPIXEL, NOSE_PIN>(noseleds, NOSE_LEDS);
  FastLED.addLeds<NEOPIXEL, TAIL_PIN>(tailleds, TAIL_LEDS);
}

void loop() {

  if (firstrun) {
    setInitPattern();
    firstrun = false;
  }

// Read in the length of the signal in microseconds
    prevch1 = ch1;
    ch1 = pulseIn(rcPin1, HIGH, 25000);  // (Pin, State, Timeout)
    //ch1 = 1500;
    if (ch1 < 700) {ch1 = prevch1;}
 
    static int prevModeIn = 0;
    static int currentModeIn = 0;

    currentModeIn = round(ch1/100);
    if (currentModeIn != prevModeIn) {
        currentShow = map(currentModeIn, 9, 19, 0, NUM_SHOWS-1);
/*        Serial.write(27);       // ESC command
        Serial.print("[2J");    // clear screen command
        Serial.write(27);
        Serial.print("[H");     // cursor to home command
        Serial.print("Current Mode: ");
        Serial.println(currentShow);
        Serial.print("Channel value: ");
        Serial.println(ch1);
        Serial.print("Brightness value: ");
        Serial.println("------------");
        //delay(200);*/
  
        // knobRemainder = ( ( (currentModeIn * NUM_SHOWS) * 100 ) / 1023 ) % 100; 
        prevModeIn = currentModeIn;
    }

    unsigned long currentMillis = millis();
  if (currentMillis - prevMillis > interval) {
        prevMillis = currentMillis;
        stepShow();
    }
}

void stepShow() {
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
//    static int prevBright = 0;
//    static int currentBright = 1;
//    if (currentBright != prevBright) {
//        FastLED.setBrightness(map(currentBright, 0, 1023, MIN_BRIGHTNESS, MAX_BRIGHTNESS));
//        prevBright = currentBright;
//    }
    #ifdef TMP_BRIGHTNESS
    FastLED.setBrightness(TMP_BRIGHTNESS);
    #endif
    FastLED.show();
}

void blank() {
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
        rightleds[i] = ColorFromPalette(palette, map(i, 0, NON_NAV_LEDS, 0, 255));
        leftleds[i] = ColorFromPalette(palette, map(i, 0, NON_NAV_LEDS, 0, 255));
        if (i <= NOSE_LEDS) {noseleds[i] = ColorFromPalette(palette, map(i, 0, NOSE_LEDS, 0, 255));}
        if (i <= FUSE_LEDS) {fuseleds[i] = ColorFromPalette(palette, map(i, 0, FUSE_LEDS, 0, 255));}
        if (i <= TAIL_LEDS) {tailleds[i] = ColorFromPalette(palette, map(i, 0, TAIL_LEDS, 0, 255));}
    }
    interval = 20;
    showStrip();
}

void setPattern (char pattern[]) {
  for (int i = 0; i < NON_NAV_LEDS; i++) {
    CRGB newColor;
    switch (pattern[i]) {
      case 'r': newColor = CRGB::Red;
                break;
      case 'g': newColor = CRGB::Green;
                break;
      case 'b': newColor = CRGB::Blue;
                break;
      case 'w': newColor = CRGB::White;
                break;
      case 'a': newColor = CRGB::AntiqueWhite;
                break;
      case 'o': newColor = CRGB::Black;
                break;
    }
    rightleds[i] = newColor;
    leftleds[i] = newColor;
  }
  interval = 20;
  showStrip();
}

void setInitPattern () {

  for (int i = 0; i < WING_LEDS; i++) {
    CRGB newColor;
    switch (init_rightwing[i]) {
      case 'r': newColor = CRGB::Red;
                break;
      case 'g': newColor = CRGB::Green;
                break;
      case 'b': newColor = CRGB::Blue;
                break;
      case 'w': newColor = CRGB::White;
                break;
      case 'a': newColor = CRGB::AntiqueWhite;
                break;
      case 'o': newColor = CRGB::Black;
                break;
    }
    rightleds[i] = newColor;
  }
  
  for (int i = 0; i < WING_LEDS; i++) {
    CRGB newColor;
    switch (init_leftwing[i]) {
      case 'r': newColor = CRGB::Red;
                break;
      case 'g': newColor = CRGB::Green;
                break;
      case 'b': newColor = CRGB::Blue;
                break;
      case 'w': newColor = CRGB::White;
                break;
      case 'a': newColor = CRGB::AntiqueWhite;
                break;
      case 'o': newColor = CRGB::Black;
                break;
    }
    leftleds[i] = newColor;
  }
  
  for (int i = 0; i < NOSE_LEDS; i++) {
    CRGB newColor;
    switch (init_nose[i]) {
      case 'r': newColor = CRGB::Red;
                break;
      case 'g': newColor = CRGB::Green;
                break;
      case 'b': newColor = CRGB::Blue;
                break;
      case 'w': newColor = CRGB::White;
                break;
      case 'a': newColor = CRGB::AntiqueWhite;
                break;
      case 'o': newColor = CRGB::Black;
                break;
    }
    noseleds[i] = newColor;
  }
  
  for (int i = 0; i < FUSE_LEDS; i++) {
    CRGB newColor;
    switch (init_fuse[i]) {
      case 'r': newColor = CRGB::Red;
                break;
      case 'g': newColor = CRGB::Green;
                break;
      case 'b': newColor = CRGB::Blue;
                break;
      case 'w': newColor = CRGB::White;
                break;
      case 'a': newColor = CRGB::AntiqueWhite;
                break;
      case 'o': newColor = CRGB::Black;
                break;
    }
    fuseleds[i] = newColor;
  }
  
  for (int i = 0; i < TAIL_LEDS; i++) {
    CRGB newColor;
    switch (init_tail[i]) {
      case 'r': newColor = CRGB::Red;
                break;
      case 'g': newColor = CRGB::Green;
                break;
      case 'b': newColor = CRGB::Blue;
                break;
      case 'w': newColor = CRGB::White;
                break;
      case 'a': newColor = CRGB::AntiqueWhite;
                break;
      case 'o': newColor = CRGB::Black;
                break;
    }
    tailleds[i] = newColor;
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
      rightleds[j] = CHSV(currentStep + (ledOffset * j), 255, 255); //200 saturation for a less intense color palette
      leftleds[j] = CHSV(currentStep + (ledOffset * j), 255, 255); //200 saturation for a less intense color palette
      if (j < FUSE_LEDS) {fuseleds[j] = CHSV(currentStep + (ledOffset * j), 255, 255);}
    }
    currentStep++;
    interval = 10;
    showStrip();
}

void chase() {
    static int chaseStep = 0;
    if (prevShow != currentShow) {blank();}
    
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
    showStrip();
    rightleds[chaseStep-1] = CRGB::Black;
    leftleds[chaseStep-1] = CRGB::Black;
    if (chaseStep < NOSE_LEDS+1) {noseleds[chaseStep-1] = CRGB::Black;}
    if (chaseStep < FUSE_LEDS+1) {fuseleds[chaseStep-1] = CRGB::Black;}
    if (chaseStep < TAIL_LEDS+1) {tailleds[chaseStep-1] = CRGB::Black;}
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
      CRGB noseColor = CRGB::Blue;
      CRGB fuseColor = CRGB::Blue;
      CRGB tailColor = CRGB::Blue;

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
  static int major_alt;
  static int minor_alt;
  double T, P;
  char result = bmp.startMeasurment();

  if (result != 0) {
    delay(result);
    result = bmp.getTemperatureAndPressure(T, P);

    if (result != 0) {
      double A = bmp.altitude(P, P0);

      major_alt = map(A, 0, 600, 0, FUSE_LEDS/3);
      for (int i=0; i < major_alt; i++) {
        rightleds[i] = CRGB::White;
        rightleds[i+1] = CRGB::White;
        rightleds[i+2] = CRGB::White;
        leftleds[i] = CRGB::White;
        leftleds[i+1] = CRGB::White;
        leftleds[i+2] = CRGB::White;
      }
      for (int i=major_alt+1; i < FUSE_LEDS; i++) {
        rightleds[i] = CRGB::Black;
        rightleds[i+1] = CRGB::Black;
        rightleds[i+2] = CRGB::Black;
        leftleds[i] = CRGB::Black;
        leftleds[i+1] = CRGB::Black;
        leftleds[i+2] = CRGB::Black;
      }
      
    }
  }
  interval = 500;
}


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
          noseleds[i] = colorMin;
          fuseleds[i] = colorMin;
          tailleds[i] = colorMin;
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


// void solidColor (int amount) {
//     CRGB color;
//     if (amount < 10) {
//         color = CRGB(255, 255, 255); //pure white
//     } else if (amount < 20) {
//         color = CRGB(255, 172, 68); //warm white
//     } else {
//         color = CHSV(map(amount, 20, 100, 0, 255), 255, 255);
//     }

//     for (int i = 0; i < WING_LEDS; i++) {
//         leds[i] = color;
//     }
//     interval = 20;
//     showStrip();
// }

