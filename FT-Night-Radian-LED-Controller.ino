// #include "src/FastLED/FastLED.h"
// #include "src/Adafruit_BMP280_Library/Adafruit_BMP280.h"
#include <FastLED.h>
#include <Adafruit_BMP280.h>
#include <EEPROM.h>
// define number of LEDs in specific strings
#define WING_LEDS 31
#define NON_NAV_LEDS 20
#define FUSE_LEDS 18
#define NOSE_LEDS 4
#define TAIL_LEDS 8
#define MIN_BRIGHTNESS 32
#define MAX_BRIGHTNESS 255
//#define TMP_BRIGHTNESS 55
#define MAX_ALTIMETER 400
#define PROGRAM_CYCLE_BTN 6
#define PROGRAM_ENABLE_BTN 7

// define the pins that the LED strings are connected to
#define TAIL_PIN 8
#define FUSE_PIN 9
#define NOSE_PIN 10
#define LEFT_PIN 11
#define RIGHT_PIN 12

#define RC_PIN1 5   // Pin 5 Connected to Receiver;
#define RC_PIN2 4   // Pin 4 Connected to Receiver for optional second channel;
#define NUM_SHOWS 18

#define CONFIG_VERSION 0xAA01 // EEPROM config version (increment this any time the Config struct changes).
#define CONFIG_START 0 // starting EEPROM address for our config

#define METRIC_CONVERSION 3.3;

#define caseshow(x,y) case x: y; break;

uint8_t wingNavPoint = NON_NAV_LEDS;

uint8_t activeShowNumbers[NUM_SHOWS]; // our array of currently active show numbers
uint8_t numActiveShows = NUM_SHOWS; // how many actual active shows

float basePressure;
double fakeAlt = 0;

int currentCh1 = 0;  // Receiver Channel PPM value
int prevCh1 = 0; // determine if the Receiver signal changed
bool programMode = false;

CRGB rightleds[WING_LEDS];
CRGB leftleds[WING_LEDS];
CRGB noseleds[NOSE_LEDS];
CRGB fuseleds[FUSE_LEDS];
CRGB tailleds[TAIL_LEDS];

uint8_t currentShow = 0; // which LED show are we currently running
uint8_t prevShow = 0; // did the LED show change
int currentStep = 0;

unsigned long prevMillis = 0;
unsigned long prevNavMillis = 0;
unsigned long progMillis = 0;
unsigned long prevStrobeMillis = 0;

int interval;

Adafruit_BMP280 bmp;


//       _        _   _                    _   _                       
//   ___| |_ __ _| |_(_) ___   _ __   __ _| |_| |_ ___ _ __ _ __  ___  
//  / __| __/ _` | __| |/ __| | '_ \ / _` | __| __/ _ \ '__| '_ \/ __| 
//  \__ \ || (_| | |_| | (__  | |_) | (_| | |_| ||  __/ |  | | | \__ \ 
//  |___/\__\__,_|\__|_|\___| | .__/ \__,_|\__|\__\___|_|  |_| |_|___/ 
//                            |_|                                      

char usflag[] = {'b', 'b', 'b', 'w', 'w', 'w', 'w', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'w', 'w', 'w', 'r', 'r', 'r', 'b', 'b', 'b'};
char init_rightwing[] = {'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g'};
char rightwing[] = {'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g', 'g'};
char init_leftwing[] = {'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r'};
char leftwing[] = {'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r'};
char init_nose[] = {'b', 'b', 'b', 'b'};
char init_fuse[] = {'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b'};
char init_tail[] = {'b', 'b', 'b', 'b', 'b', 'b', 'b', 'b'};
char christmas[] = {'r', 'r', 'r', 'g', 'g', 'g', 'g', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'g', 'g', 'g', 'r', 'r', 'r', 'g', 'g', 'g'};

//                       _ _            _        
//    __ _ _ __ __ _  __| (_) ___ _ __ | |_ ___  
//   / _` | '__/ _` |/ _` | |/ _ \ '_ \| __/ __| 
//  | (_| | | | (_| | (_| | |  __/ | | | |_\__ \ 
//   \__, |_|  \__,_|\__,_|_|\___|_| |_|\__|___/ 
//   |___/                                       

DEFINE_GRADIENT_PALETTE( orange_yellow ) {  //RGB(255,165,0) RGB(255,244,175) RGB(255,246,0)
  0,   255,165,  0,   //orange
128,   255,244,175,   //bright white-yellow
255,   255,246,  0 }; //

DEFINE_GRADIENT_PALETTE( teal_blue ) {      //RGB(0,244,216) RGB(48,130,219)
  0,     0,244,216,   //tealish
255,    48,130,219 }; //blueish

DEFINE_GRADIENT_PALETTE( blue_black ) {     //RGB(0,0,0) RGB(0,0,255)
  0,    0,0,0,
255,    0,0,255 };

DEFINE_GRADIENT_PALETTE( variometer ) {     //RGB(255,0,0) RGB(255,255,255) RGB(0,255,0)
  0,    255,0,0, //Red
118,    255,255,255, //White
138,    255,255,255, //White
255,    0,255,0 }; //Green


//   ___  ___ _ __  _ __ ___  _ __ ___  
//  / _ \/ _ \ '_ \| '__/ _ \| '_ ` _ \ 
// |  __/  __/ |_) | | | (_) | | | | | |
//  \___|\___| .__/|_|  \___/|_| |_| |_|
//           |_|                        

struct Config { // this is the main config struct that holds everything we'd want to save/load from EEPROM
  uint16_t version;
  bool enabledShows[NUM_SHOWS];
  bool navlights;
} config;

void loadConfig() { // loads existing config from EEPROM, or if wrong version, sets up new defaults and saves them
  EEPROM.get(CONFIG_START, config);
  Serial.println(F("Loading config..."));
  if (config.version != CONFIG_VERSION) {
    // setup defaults
    config.version = CONFIG_VERSION;
    memset(config.enabledShows, true, sizeof(config.enabledShows)); // set all entries of enabledShows to true by default
    config.navlights = true;
    Serial.println(F("New config version. Setting defaults..."));
    
    saveConfig();
  } else { // only run update if we didn't just make defaults, as saveConfig() already does this
    updateShowConfig();
  }
}

void saveConfig() { // saves current config to EEPROM
  EEPROM.put(CONFIG_START, config);
  Serial.println(F("Saving config..."));
  updateShowConfig();
}

void updateShowConfig() { // sets order of currently active shows. e.g., activeShowNumbers[] = {1, 4, 5, 9}. also sets nav stop point.
  Serial.print(F("Config version: "));
  Serial.println(config.version);
  numActiveShows = 0; // using numActiveShows also as a counter in the for loop to save a variable
  for (int i = 0; i < NUM_SHOWS; i++) {
    Serial.print(F("Show "));
    Serial.print(i);
    Serial.print(F(": "));
    if (config.enabledShows[i]) {
      Serial.println(F("enabled."));
      activeShowNumbers[numActiveShows] = i;
      numActiveShows++;
    } else {
      Serial.println(F("disabled."));
    }
  }
  Serial.print(F("Navlights: "));
  if (config.navlights) {
    wingNavPoint = NON_NAV_LEDS;
    Serial.println(F("on."));
  } else {
    wingNavPoint = WING_LEDS;
    Serial.println(F("off."));
  }
}

//            _                
//   ___  ___| |_ _   _ _ __   
//  / __|/ _ \ __| | | | '_ \  
//  \__ \  __/ |_| |_| | |_) | 
//  |___/\___|\__|\__,_| .__/  
//                     |_|     

void setup() {
  Serial.begin(115200);

  loadConfig();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  bmp.begin(0x76); // initialize the altitude pressure sensor with I2C address 0x76
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,     /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X4,       /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_63); /* Standby time. */

  basePressure = bmp.readPressure()/100; // this gets the current pressure at "ground level," so we can get relative altitude

  Serial.print(F("Base Pressure: "));
  Serial.println(basePressure);

  pinMode(PROGRAM_CYCLE_BTN, INPUT_PULLUP);
  pinMode(PROGRAM_ENABLE_BTN, INPUT_PULLUP);
  pinMode(RC_PIN1, INPUT);
  pinMode(RC_PIN2, INPUT);
  FastLED.addLeds<NEOPIXEL, RIGHT_PIN>(rightleds, WING_LEDS);
  FastLED.addLeds<NEOPIXEL, LEFT_PIN>(leftleds, WING_LEDS);
  FastLED.addLeds<NEOPIXEL, FUSE_PIN>(fuseleds, FUSE_LEDS);
  FastLED.addLeds<NEOPIXEL, NOSE_PIN>(noseleds, NOSE_LEDS);
  FastLED.addLeds<NEOPIXEL, TAIL_PIN>(tailleds, TAIL_LEDS);
}

//                   _         _                    
//   _ __ ___   __ _(_)_ __   | | ___   ___  _ __   
//  | '_ ` _ \ / _` | | '_ \  | |/ _ \ / _ \| '_ \  
//  | | | | | | (_| | | | | | | | (_) | (_) | |_) | 
//  |_| |_| |_|\__,_|_|_| |_| |_|\___/ \___/| .__/  
//                                          |_|     

void loop() {
  static bool firstrun = true;
  static int programModeCounter = 0;
  static int enableCounter = 0;
  static unsigned long currentMillis = millis();

  if (firstrun) {
    setInitPattern(); // Set the LED strings to their boot-up configuration
    firstrun = false;
  }
  
  // The timing control for calling each "frame" of the different animations
  currentMillis = millis();
  if (currentMillis - prevMillis > interval) {
    prevMillis = currentMillis;
    stepShow();
  }

  if (config.navlights) { // navlights if enabled
    if (currentMillis - prevNavMillis > 30) {
      prevNavMillis = currentMillis;
        navLights();
    }
  }

  if (programMode) { // we are in program mode where the user can enable/disable programs and set parameters
    // Are we exiting program mode?
    if (digitalRead(PROGRAM_CYCLE_BTN) == LOW) { // Is the Program button pressed?
      programModeCounter = programModeCounter + (currentMillis - progMillis); // increment the counter by how many milliseconds have passed
      //Serial.println(programModeCounter);
      if (programModeCounter > 3000) { // Has the button been held down for 5 seconds?
        programMode = false;
        Serial.println(F("Exiting program mode"));
        // store current program values into eeprom
        saveConfig();
        programModeCounter = 0;
        prevCh1 = -1;
        programInit('w'); //strobe the leds to indicate leaving program mode
      }
    } else {
      if (programModeCounter > 0 && programModeCounter < 1000) { // a momentary press to cycle to the next program
        currentShow++;
        if (currentShow == NUM_SHOWS) {currentShow = 0;}
      }
      programModeCounter = 0;
    }
    
    if (digitalRead(PROGRAM_ENABLE_BTN) == LOW) { // Is the Program Enable button pressed?
      enableCounter = enableCounter + (currentMillis - progMillis); // increment the counter by how many milliseconds have passed
    } else {
      if (enableCounter > 0 && enableCounter < 1000) { // momentary press to toggle the current show
        //toggle the state of the current program, currState = !currState
        config.enabledShows[currentShow] = !config.enabledShows[currentShow];
        programInit(config.enabledShows[currentShow]);
      }
      enableCounter = 0;
    }

  } else { // we are not in program mode. Read signal from receiver and run through programs normally.
    // Read in the length of the signal in microseconds
    prevCh1 = currentCh1;
    currentCh1 = pulseIn(RC_PIN1, HIGH, 25000);  // (Pin, State, Timeout)
    // currentCh1 = 900;
    if (currentCh1 != prevCh1) {
      if (currentCh1 < 700) {currentCh1 = prevCh1;} // if signal is lost or poor quality, we continue running the same show
      currentShow = map(currentCh1, 900, 1900, 0, numActiveShows-1); // mapping 9-19 to get the 900ms - 1900ms value
      // currentShow = constrain(currentShow, 0, numActiveShows-1);
      currentShow = currentShow % numActiveShows;
      //currentShow = 17;  // uncomment these two lines to test the altitude program using the xmitter knob to drive the altitude reading
      //fakeAlt = map(currentCh1, 900, 1900, 0, MAX_ALTIMETER);
    }
    
    // Are we entering program mode?
    if (digitalRead(PROGRAM_CYCLE_BTN) == LOW) { // Is the Program button pressed?
      programModeCounter = programModeCounter + (currentMillis - progMillis); // increment the counter by how many milliseconds have passed
      //Serial.println(programModeCounter);
      if (programModeCounter > 3000) { // Has the button been held down for 5 seconds?
        programMode = true;
        programModeCounter = 0;
        Serial.println(F("Entering program mode"));
        programInit('w'); //strobe the leds to indicate entering program mode
        currentShow = 0;
        programInit(config.enabledShows[currentShow]);
      }
    } else if (digitalRead(PROGRAM_ENABLE_BTN) == LOW) {
      programModeCounter = programModeCounter + (currentMillis - progMillis);
      if (programModeCounter > 3000) {
        config.navlights = !config.navlights;
        saveConfig();
        programModeCounter = 0;
      }
    } else {
      programModeCounter = 0;
    }
  }
  progMillis = currentMillis;
}

//       _                   _                    
//   ___| |_ ___ _ __    ___| |__   _____      __ 
//  / __| __/ _ \ '_ \  / __| '_ \ / _ \ \ /\ / / 
//  \__ \ ||  __/ |_) | \__ \ | | | (_) \ V  V /  
//  |___/\__\___| .__/  |___/_| |_|\___/ \_/\_/   
//              |_|                               

void stepShow() { // the main menu of different shows
  if (currentShow != prevShow) {
    Serial.print(F("Current Show: "));
    Serial.println(currentShow);
    currentStep = 0;
    blank();
    if (programMode) {
      //Look up whether this currentShow is enabled or disabled, and flash the LEDs accordingly
      if (config.enabledShows[currentShow]) { // this should now check EEPROM config
        programInit('g'); //flash all LEDs green to indicate current show is enabled
      } else {
        programInit('r'); //flash all LEDs red to indicate current show is disabled
      }
    }
  }

  int switchShow;
  if (programMode) {
    switchShow = currentShow;
  } else {
    switchShow = activeShowNumbers[currentShow];
  }

  switch (switchShow) { // activeShowNumbers[] will look like {1, 4, 5, 9}, so this maps to actual show numbers
    caseshow(0, blank()); //all off
    caseshow(1, colorWave1(10, 10));//regular rainbow
    caseshow(2, colorWave1(0, 10)); // whole plane solid color rainbow
    caseshow(3, setColor(CRGB::Red));
    caseshow(4, setColor(CRGB::Orange));
    caseshow(5, setColor(CRGB::Yellow));
    caseshow(6, setColor(CRGB::Green));
    caseshow(7, setColor(CRGB::Blue));
    caseshow(8, setColor(CRGB::Indigo));
    caseshow(9, setColor(CRGB::DarkCyan));
    caseshow(10, setColor(CRGB::White));
    caseshow(11, twinkle1()); //twinkle effect
    caseshow(12, strobe(3)); //Realistic double strobe alternating between wings
    caseshow(13, strobe(2)); //Realistic landing-light style alternating between wings
    caseshow(14, strobe(1)); // unrealistic rapid strobe of all non-nav leds, good locator/identifier
    caseshow(15, chase(CRGB::White, CRGB::Black, 8, 7, 3, 50));
    caseshow(16, cylon(CRGB::Orange, CRGB::DarkCyan, 8, 7, 3, 50));
            /*TODO Chase programs:
            Chase all on but a few off. 
            Chase all off but a few on.
            Chase all out from center.
            Chase all in to center.
            Chase forward.
            Chase rearward.
             */
    caseshow(17, altitude(fakeAlt, variometer)); // fakeAlt is for testing. Defaults to zero for live data.
  }
  prevShow = currentShow;
}

//   _          _                    __                  _   _                  
//  | |__   ___| |_ __   ___ _ __   / _|_   _ _ __   ___| |_(_) ___  _ __  ___  
//  | '_ \ / _ \ | '_ \ / _ \ '__| | |_| | | | '_ \ / __| __| |/ _ \| '_ \/ __| 
//  | | | |  __/ | |_) |  __/ |    |  _| |_| | | | | (__| |_| | (_) | | | \__ \ 
//  |_| |_|\___|_| .__/ \___|_|    |_|  \__,_|_| |_|\___|\__|_|\___/|_| |_|___/ 
//               |_|                                                            

void showStrip () {
  #ifdef TMP_BRIGHTNESS
  FastLED.setBrightness(TMP_BRIGHTNESS);
  #endif
  FastLED.show();
}

void blank() { // Turn off all LEDs
  setColor(CRGB::Black);
  showStrip();
}

void setColor (CRGB color) {
  fill_solid(rightleds, wingNavPoint, color);
  fill_solid(leftleds, wingNavPoint, color);
  fill_solid(noseleds, NOSE_LEDS, color);
  fill_solid(fuseleds, FUSE_LEDS, color);
  fill_solid(tailleds, TAIL_LEDS, color);
  showStrip();
}

void setColor (CRGBPalette16 palette) {
  for (int i; i < wingNavPoint; i++) {
    rightleds[i] = ColorFromPalette(palette, map(i, 0, wingNavPoint, 0, 240));
    leftleds[i] = ColorFromPalette(palette, map(i, 0, wingNavPoint, 0, 240));
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

void setPattern (char pattern[]) {
  for (int i = 0; i < wingNavPoint; i++) {
    rightleds[i] = LetterToColor(pattern[i]);
    leftleds[i] = LetterToColor(pattern[i]);
  }
  interval = 20;
  showStrip();
}

void setInitPattern () {
  for (int i = 0; i < WING_LEDS; i++) {
    rightleds[i] = LetterToColor(init_rightwing[i]);
  }
  
  for (int i = 0; i < WING_LEDS; i++) {
    leftleds[i] = LetterToColor(init_leftwing[i]);
  }
  
  for (int i = 0; i < NOSE_LEDS; i++) {
    noseleds[i] = LetterToColor(init_nose[i]);
  }
  
  for (int i = 0; i < FUSE_LEDS; i++) {
    fuseleds[i] = LetterToColor(init_fuse[i]);
  }
  
  for (int i = 0; i < TAIL_LEDS; i++) {
    tailleds[i] = LetterToColor(init_tail[i]);
  }
  
  showStrip();
}

void animateColor (CRGBPalette16 palette, int ledOffset, int stepSize) {
  if (currentStep > 255) {currentStep = 0;}
  for (int i = 0; i < wingNavPoint; i++) {
      int j = triwave8((i * ledOffset) + currentStep);
      rightleds[i] = ColorFromPalette(palette, scale8(j, 240));
      leftleds[i] = ColorFromPalette(palette, scale8(j, 240));
  }

  currentStep += stepSize;
  interval = 20;
  showStrip();
}

void setFuseLeds(uint8_t led, CRGB color) { // sets leds along nose and fuse as if they were the same strip. range is 0 - ((NOSE_LEDS+FUSE_LEDS)-1)
  if (led < NOSE_LEDS) {
    noseleds[led] = color; // this is if the nose strip is reversed. can't remember if it is or not. otherwise, noseleds[led]
  } else {
    fuseleds[led-NOSE_LEDS] = color;
  }
}

//               _                 _   _                  
//    __ _ _ __ (_)_ __ ___   __ _| |_(_) ___  _ __  ___  
//   / _` | '_ \| | '_ ` _ \ / _` | __| |/ _ \| '_ \/ __| 
//  | (_| | | | | | | | | | | (_| | |_| | (_) | | | \__ \ 
//   \__,_|_| |_|_|_| |_| |_|\__,_|\__|_|\___/|_| |_|___/ 

void colorWave1 (uint8_t ledOffset, uint8_t l_interval) { // Rainbow pattern on wings and fuselage
  if (currentStep > 255) {currentStep = 0;}
  for (int j = 0; j < wingNavPoint; j++) {
    rightleds[j] = CHSV(currentStep + (ledOffset * j), 255, 255);
    leftleds[j] = CHSV(currentStep + (ledOffset * j), 255, 255);
    if (j < FUSE_LEDS) {fuseleds[j] = CHSV(currentStep + (ledOffset * j), 255, 255);}
  }
  currentStep++;
  interval = l_interval;
  showStrip();
}

void chase(CRGB color1, CRGB color2, uint8_t lengthWing, uint8_t lengthFuse, uint8_t lengthTail, uint8_t l_interval) { // color segment that chases through the wings
  static uint8_t currentStepFuse = 0;
  static uint8_t currentStepTail = 0;
  //currentStep is for wings

  if (currentStep >= wingNavPoint) {currentStep = 0;}
  if (currentStepFuse >= (NOSE_LEDS+FUSE_LEDS)) {currentStepFuse = 0;}
  if (currentStepTail >= TAIL_LEDS) {currentStepTail = 0;}

  setColor(color2); // sets the base/background color

  int8_t j;
  CRGB fadeColor;
  for (uint8_t i = 0; i < lengthWing; i++) { // this iterates through the "trail", which fades behind the chase point
    fadeColor = color1.lerp8(color2, (255 / (lengthWing - 1)) * i); // fades between the chase point color and the background, based on the position in the trail

    j = currentStep - i;
    if (j < 0) {
      rightleds[wingNavPoint + j] = fadeColor;
      leftleds[wingNavPoint + j] = fadeColor;
    } else {
      rightleds[j] = fadeColor;
      leftleds[j] = fadeColor;
    }
  }
  for (uint8_t i = 0; i < lengthFuse; i++) {
    fadeColor = color1.lerp8(color2, (255 / (lengthFuse - 1)) * i);

    j = currentStepFuse - i;
    if (j < 0) {
      setFuseLeds((NOSE_LEDS+FUSE_LEDS) + j, fadeColor);
    } else {
      setFuseLeds(j, fadeColor);
    }
  }
  for (uint8_t i = 0; i < lengthTail; i++) {
    fadeColor = color1.lerp8(color2, (255 / (lengthTail - 1)) * i);

    j = currentStepTail - i;
    if (j < 0) {
      tailleds[TAIL_LEDS + j] = fadeColor;
    } else {
      tailleds[j] = fadeColor;
    }
  }

  currentStep++;
  currentStepFuse++;
  currentStepTail++;
  interval = l_interval;
  showStrip();
}

void cylon(CRGB color1, CRGB color2, uint8_t lengthWing, uint8_t lengthFuse, uint8_t lengthTail, uint8_t l_interval) {
  static int8_t direction = 1;
  static int8_t directionFuse = 1;
  static int8_t directionTail = 1;

  static int8_t currentStepFuse = 0;
  static int8_t currentStepTail = 0;

  setColor(color2); // sets the base/background color

  if (currentStep >= wingNavPoint) {
    currentStep = wingNavPoint - 1;
    direction = -1;
  }
  if (currentStep < 0) {
    currentStep = 1;
    direction = 1;
  }

  if (currentStepFuse >= (NOSE_LEDS+FUSE_LEDS)) {
    currentStepFuse = (NOSE_LEDS+FUSE_LEDS) - 1;
    directionFuse = -1;
  }
  if (currentStepFuse < 0) {
    currentStepFuse = 1;
    directionFuse = 1;
  }

  if (currentStepTail >= TAIL_LEDS) {
    currentStepTail = TAIL_LEDS - 1;
    directionTail = -1;
  }
  if (currentStepTail < 0) {
    currentStepTail = 1;
    directionTail = 1;
  }

  int8_t j;
  CRGB fadeColor;
  for (int8_t i = (lengthWing - 1); i >= 0; i--) {
    fadeColor = color1.lerp8(color2, (255 / (lengthWing - 1)) * i); // fades between the chase point color and the background, based on the position in the trail

    j = currentStep - (i * direction);
    if (j < 0) {j = -j;}
    if (j > (wingNavPoint - 1)) {j = wingNavPoint - i;}
    rightleds[j] = fadeColor;
    leftleds[j] = fadeColor;
  }

  for (int8_t i = (lengthFuse - 1); i >= 0; i--) {
    fadeColor = color1.lerp8(color2, (255 / (lengthFuse - 1)) * i); // fades between the chase point color and the background, based on the position in the trail

    j = currentStepFuse - (i * directionFuse);
    if (j < 0) {j = -j;}
    if (j > ((NOSE_LEDS+FUSE_LEDS) - 1)) {j = (NOSE_LEDS+FUSE_LEDS) - i;}
    setFuseLeds(j, fadeColor);
  }

  for (int8_t i = (lengthTail - 1); i >= 0; i--) {
    fadeColor = color1.lerp8(color2, (255 / (lengthTail - 1)) * i); // fades between the chase point color and the background, based on the position in the trail

    j = currentStepTail - (i * directionTail);
    if (j < 0) {j = -j;}
    if (j > (TAIL_LEDS - 1)) {j = TAIL_LEDS - i;}
    tailleds[j] = fadeColor;
  }

  currentStep += direction;
  currentStepFuse += directionFuse;
  currentStepTail += directionTail;
  interval = l_interval;
  showStrip();
}

void setNavLeds(const struct CRGB& rcolor, const struct CRGB& lcolor) { // helper function for the nav lights
  for (int i = wingNavPoint; i < WING_LEDS; i++) {
    rightleds[i] = rcolor;
    leftleds[i] = lcolor;
  }
}

//TODO re-write navlights and all function logic to do a white tail when navlights are on.
//     Similar to the wingNavPoint setup
void navLights() { // persistent nav lights
static int navStrobeState = 0;
  switch(navStrobeState) {
    case 0:
      // red/green
      setNavLeds(CRGB::Red, CRGB::Green);
      break;
    case 50:
      // strobe 1
      setNavLeds(CRGB::White, CRGB::White);
      break;
    case 52:
      // back to red/green
      setNavLeds(CRGB::Red, CRGB::Green);
      break;
    case 54:
      // strobe 2
      setNavLeds(CRGB::White, CRGB::White);
      break;
    case 56:
      // red/green again
      setNavLeds(CRGB::Red, CRGB::Green);
      navStrobeState = 0;
      break;
  }
  showStrip();
  navStrobeState++;
}

// TODO: maybe re-write some of the strobe functions in the style of the navlight "animation",
//       with a counter and a "frame" switchcase.
void strobe(int style) { // Various strobe patterns (duh)
  static bool StrobeState = true;

  switch(style) {

    case 1: //Rapid strobing all LEDS in unison
      if (StrobeState) {
        for (int i = 0; i < wingNavPoint; i++) {
          rightleds[i] = CRGB::White;
          leftleds[i] = CRGB::White;
        }
        for (int i = 0; i < NOSE_LEDS; i++) {noseleds[i] = CRGB::White;}
        for (int i = 0; i < FUSE_LEDS; i++) {fuseleds[i] = CRGB::White;}
        for (int i = 0; i < TAIL_LEDS; i++) {tailleds[i] = CRGB::White;}
        StrobeState = false;
      } else {
        for (int i = 0; i < wingNavPoint; i++) {
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
        for (int i = 0; i < wingNavPoint; i++) {
          rightleds[i] = CRGB::White;
          leftleds[i] = CRGB::Black;
        }
        for (int i = 0; i < NOSE_LEDS; i++) {noseleds[i] = CRGB::Blue;}
        for (int i = 0; i < FUSE_LEDS; i++) {fuseleds[i] = CRGB::Blue;}
        for (int i = 0; i < TAIL_LEDS; i++) {tailleds[i] = CRGB::White;}
      } else {
        for (int i = 0; i < wingNavPoint; i++) {
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

      switch(currentStep) {

        case 0: // Right wing on for 50ms
          for (int i = 0; i < wingNavPoint; i++) {
            rightleds[i] = CRGB::White;
            leftleds[i] = CRGB::Black;
          }
          interval = 50;
        break;
          
        case 1: // Both wings off for 50ms
          for (int i = 0; i < wingNavPoint; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::Black;
          }
          interval = 50;
        break;
          
        case 2: // Right wing on for 50ms
          for (int i = 0; i < wingNavPoint; i++) {
            rightleds[i] = CRGB::White;
            leftleds[i] = CRGB::Black;
          }
          interval = 50;
        break;
          
        case 3: // Both wings off for 500ms
          for (int i = 0; i < wingNavPoint; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::Black;
          }
          interval = 500;
        break;
          
        case 4: // Left wing on for 50ms
          for (int i = 0; i < wingNavPoint; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::White;
          }
          interval = 50;
        break;
          
        case 5: // Both wings off for 50ms
          for (int i = 0; i < wingNavPoint; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::Black;
          }
          interval = 50;
        break;
          
        case 6: // Left wing on for 50ms
          for (int i = 0; i < wingNavPoint; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::White;
          }
          interval = 50;
        break;
          
        case 7: // Both wings off for 500ms
          for (int i = 0; i < wingNavPoint; i++) {
            rightleds[i] = CRGB::Black;
            leftleds[i] = CRGB::Black;
          }
          interval = 500;
        break;
                    
      }

      showStrip();
      currentStep++;
      if (currentStep == 8) {currentStep = 0;}
    break;

  }
}

// TODO: Do a full flight test to make sure this function still works properly.
//       Also, clean up the commented code? Not sure if it might be needed again.
void altitude(double fake, CRGBPalette16 palette) { // Altitude indicator show. 
  // static int majorAlt;
  // static int minorAlt;
  static double prevAlt;
  static int avgVSpeed[] = {0,0,0,0};

  interval = 100; // we're going to use interval as a "delta" to base the vspeed off of

  int vSpeed;
  double currentAlt;

  currentAlt = bmp.readAltitude(basePressure)*METRIC_CONVERSION;
  //if (currentAlt < 0) {currentAlt = 0;}
  
  if (fake != 0) {currentAlt = fake;}

  /*  majorAlt = floor(currentAlt/100.0)*3;
  //Serial.println(majorAlt);
  minorAlt = int(currentAlt) % 100;
  minorAlt = map(minorAlt, 0, 100, 0, wingNavPoint);
  
  for (int i=0; i < minorAlt; i++) {
    rightleds[i] = CRGB::White;
    leftleds[i] = CRGB::White;
  }
  for (int i=minorAlt+1; i <= FUSE_LEDS; i++) {
    rightleds[i] = CRGB::Black;
    leftleds[i] = CRGB::Black;
  }

  for (int i=0; i < majorAlt; i++) {
    fuseleds[i-2] = CRGB::White;
    fuseleds[i-1] = CRGB::White;
    fuseleds[i] = CRGB::White;
  }
  for (int i=majorAlt+1; i < FUSE_LEDS; i++) {
    fuseleds[i-2] = CRGB::Black;
    fuseleds[i-1] = CRGB::Black;
    fuseleds[i] = CRGB::Black;
  }*/

  //Rewrite of the altitude LED graph. Wings and Fuse all graphically indicate relative altitude AGL from zero to MAX_ALTIMETER
  if (currentAlt > MAX_ALTIMETER) {currentAlt = MAX_ALTIMETER;}
  
  for (int i=0; i < map(currentAlt, 0, MAX_ALTIMETER, 0, wingNavPoint); i++) {
    rightleds[i] = CRGB::White;
    leftleds[i] = CRGB::White;
  }
  for (int i=map(currentAlt, 0, MAX_ALTIMETER, 0, wingNavPoint); i < wingNavPoint; i++) {
    rightleds[i] = CRGB::Black;
    leftleds[i] = CRGB::Black;
  }
  for (int i=0; i < map(currentAlt, 0, MAX_ALTIMETER, 0, FUSE_LEDS); i++) {
    fuseleds[i] = CRGB::White;
  }
  for (int i=map(currentAlt, 0, MAX_ALTIMETER, 0, FUSE_LEDS); i < FUSE_LEDS; i++) {
    fuseleds[i] = CRGB::Black;
  }

  //map vertical speed value to gradient palette
  int vspeedMap;
  avgVSpeed[0]=avgVSpeed[1];
  avgVSpeed[1]=avgVSpeed[2];
  avgVSpeed[2]=(currentAlt-prevAlt)*100; // *100 just gets things into int territory so we can work with it easier
  vSpeed = (avgVSpeed[0]+avgVSpeed[1]+avgVSpeed[2])/3;
  vSpeed = constrain(vSpeed, -interval, interval);

  vspeedMap = map(vSpeed, -interval, interval, 0, 240);

  for (int i = 0; i < TAIL_LEDS; i++) {
    tailleds[i] = ColorFromPalette(palette, vspeedMap);
  }

  Serial.print(F("Current relative altitude:  "));
  Serial.print(currentAlt);
  Serial.print(F("\t\tVSpeed: "));
  Serial.print(vSpeed);
  Serial.print(F("\tVSpeedMap: "));
  Serial.println(vspeedMap);
  
  prevAlt = currentAlt;

  showStrip();
}

enum {SteadyDim, Dimming, Brightening};
void doTwinkle1(struct CRGB * ledArray, uint8_t * pixelState, uint8_t size) {
  const CRGB colorDown = CRGB(1, 1, 1);
  const CRGB colorUp = CRGB(8, 8, 8);
  const CRGB colorMax = CRGB(128, 128, 128);
  const CRGB colorMin = CRGB(4, 4, 4);
  const uint8_t twinkleChance = 1;

  for (int i = 0; i < size; i++) {
    if (pixelState[i] == SteadyDim) {
      if (random8() < twinkleChance) {
        pixelState[i] = Brightening;
      }
      if (prevShow != currentShow) { // Reset all LEDs at start of show
        ledArray[i] = colorMin;
      }
    }

    if (pixelState[i] == Brightening) {
      if (ledArray[i] >= colorMax) {
        pixelState[i] = Dimming;
      } else {
        ledArray[i] += colorUp;
      }
    }

    if (pixelState[i] == Dimming) {
      if (ledArray[i] <= colorMin) {
        ledArray[i] = colorMin;
        pixelState[i] = SteadyDim;
      } else {
        ledArray[i] -= colorDown;
      }
    }
  }
}

void twinkle1 () { // Random twinkle effect on all LEDs
  static uint8_t pixelStateRight[WING_LEDS];
  static uint8_t pixelStateLeft[WING_LEDS];
  static uint8_t pixelStateNose[NOSE_LEDS];
  static uint8_t pixelStateFuse[FUSE_LEDS];
  static uint8_t pixelStateTail[TAIL_LEDS];

  if (prevShow != currentShow) { // Reset everything at start of show
    memset(pixelStateRight, SteadyDim, sizeof(pixelStateRight));
    memset(pixelStateLeft, SteadyDim, sizeof(pixelStateLeft));
    memset(pixelStateNose, SteadyDim, sizeof(pixelStateNose));
    memset(pixelStateFuse, SteadyDim, sizeof(pixelStateFuse));
    memset(pixelStateTail, SteadyDim, sizeof(pixelStateTail));
  }

  doTwinkle1(rightleds, pixelStateRight, wingNavPoint);
  doTwinkle1(leftleds,  pixelStateLeft, wingNavPoint);
  doTwinkle1(noseleds,  pixelStateNose, NOSE_LEDS);
  doTwinkle1(fuseleds,  pixelStateFuse, FUSE_LEDS);
  doTwinkle1(tailleds,  pixelStateTail, TAIL_LEDS);

  interval = 10;
  showStrip();
}

void programInit(bool progState) {
  if (progState) {
    Serial.println(F("enabled."));
    programInit('g');
  } else {
    Serial.println(F("disabled."));
    programInit('r');
  }
}

void programInit(char progState) {
  CRGB color;
  switch (progState) {
    case 'w':
      color = CRGB::White;
      break;
    case 'g':
      color = CRGB::Green;
      break;
    case 'r':
      color = CRGB::Red;
      break;
  }
  static bool StrobeState = true;
  for (int j = 0; j < 10; j++) {
      if (StrobeState) {
        for (int i = 0; i < wingNavPoint; i++) {
          rightleds[i] = color;
          leftleds[i] = color;
        }
        for (int i = 0; i < NOSE_LEDS; i++) {noseleds[i] = color;}
        for (int i = 0; i < FUSE_LEDS; i++) {fuseleds[i] = color;}
        for (int i = 0; i < TAIL_LEDS; i++) {tailleds[i] = color;}
        digitalWrite(LED_BUILTIN, HIGH);
        StrobeState = false;
      } else {
        for (int i = 0; i < wingNavPoint; i++) {
          rightleds[i] = CRGB::Black;
          leftleds[i] = CRGB::Black;
        }
        for (int i = 0; i < NOSE_LEDS; i++) {noseleds[i] = CRGB::Black;}
        for (int i = 0; i < FUSE_LEDS; i++) {fuseleds[i] = CRGB::Black;}
        for (int i = 0; i < TAIL_LEDS; i++) {tailleds[i] = CRGB::Black;}
        digitalWrite(LED_BUILTIN, LOW);
        StrobeState = true;
        }
      delay(50);
      showStrip();
  }
  digitalWrite(LED_BUILTIN, LOW);
}
