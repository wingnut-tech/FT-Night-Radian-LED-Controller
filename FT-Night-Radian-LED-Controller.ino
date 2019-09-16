#include <FastLED.h>
#include <Adafruit_BMP280.h>
#include <EEPROM.h>

#include "config.h"

#define MIN_BRIGHTNESS 32
#define MAX_BRIGHTNESS 255

//#define TMP_BRIGHTNESS 55 // uncomment to override brightness for testing

#define MAX_ALTIMETER 400

// define the pins that the buttons are connected to

#define PROGRAM_CYCLE_BTN 6
#define PROGRAM_ENABLE_BTN 7

// define the pins that the LED strings are connected to

#define TAIL_PIN 8
#define FUSE_PIN 9
#define NOSE_PIN 10
#define LEFT_PIN 11
#define RIGHT_PIN 12

// define the pins that are used for RC inputs

#define RC_PIN1 5   // Pin 5 Connected to Receiver;
#define RC_PIN2 4   // Pin 4 Connected to Receiver for optional second channel;


#define NUM_SHOWS_WITH_ALTITUDE 20 // total number of shows. 1+the last caseshow number

#define CONFIG_VERSION 0xAA03 // EEPROM config version (increment this any time the Config struct changes).
#define CONFIG_START 0 // starting EEPROM address for our config

#define METRIC_CONVERSION 3.3; // 3.3 to convert meters to feet. 1 for meters.

#define caseshow(x,y) case x: y; break; // macro for switchcases with a built-in break

uint8_t NUM_SHOWS = NUM_SHOWS_WITH_ALTITUDE; // NUM_SHOWS becomes 1 less if no BMP280 module is installed

uint8_t wingNavPoint = WING_LEDS; // the end point of the wing leds, depending on navlights being on/off

const uint8_t maxLeds = max(WING_LEDS, max(NOSE_LEDS, max(FUSE_LEDS, TAIL_LEDS)));

uint8_t activeShowNumbers[NUM_SHOWS_WITH_ALTITUDE]; // our array of currently active show switchcase numbers
uint8_t numActiveShows = NUM_SHOWS; // how many actual active shows

float basePressure; // gets initialized with ground level pressure on startup

uint8_t rcInputPort = 0; // which RC input port is plugged in? 0 watches both 1 and 2, until either one gets a valid signal. then this gets set to that number
int currentCh1 = 0;  // Receiver Channel PPM value
int currentCh2 = 0;  // Receiver Channel PPM value
bool programMode = false; // are we in program mode?

uint8_t currentShow = 0; // which LED show are we currently running
uint8_t prevShow = 0; // did the LED show change

int currentStep = 0; // this is just a global general-purpose counter variable that any show can use for whatever it needs

unsigned long prevMillis = 0; // keeps track of last millis value for regular show timing
unsigned long prevNavMillis = 0; // keeps track of last millis value for the navlights
unsigned long progMillis = 0; // keeps track of last millis value for button presses

int interval; // delay time between each "frame" of an animation

Adafruit_BMP280 bmp; // bmp280 module object

class LED {
public:
  CRGB* leds;
  bool reversed;
  uint8_t numLeds;

  // constructor, runs when first initialized
  LED(uint8_t num, bool rev) {
    reversed = rev;
    numLeds = num;
    leds = (CRGB *)malloc(sizeof(CRGB)*numLeds); // this sets up the array to be the correct length
  }

  // for setting wingNavPoint
  void stopPoint(uint8_t stop) {
    numLeds = stop;
  }

  // regular led assignment
  void set(uint8_t led, CRGB color) {
    if (led < numLeds) {
      if (reversed) {
        leds[numLeds - led - 1] = color;
      } else {
        leds[led] = color;
      }
    }
  }

  // adds color to existing value
  void add(uint8_t led, CRGB color) {
    if (led < numLeds) {
      if (reversed) {
        leds[numLeds - led - 1] += color;
      } else {
        leds[led] += color;
      }
    }
  }

  // "or"s the colors, making the led the brighter of the two
  void addor(uint8_t led, CRGB color) {
    if (led < numLeds) {
      if (reversed) {
        leds[numLeds - led - 1] |= color;
      } else {
        leds[led] += color;
      }
    }
  }

  void nscale8(uint8_t scale) {
    for (uint8_t i = 0; i < numLeds; i++) {
      leds[i] = leds[i].nscale8(scale);
    }
  }

  void lerp8(CRGB other, uint8_t frac) {
    for (uint8_t i = 0; i < numLeds; i++) {
      leds[i] = leds[i].lerp8(other, frac);
    }
  }
};

LED Right(WING_LEDS, RIGHT_REV);
LED Left(WING_LEDS, LEFT_REV);
LED Nose(NOSE_LEDS, NOSE_REV);
LED Fuse(FUSE_LEDS, FUSE_REV);
LED Tail(TAIL_LEDS, TAIL_REV);

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

DEFINE_GRADIENT_PALETTE( USA ) {           //RGB(255,0,0) RGB(255,255,255) RGB(0,0,255)
  0,   255,0,  0,   //red
 25,   255,0,  0,   //red
128,   255,255,255,   //white
220,   0,0,255,   //blue
255,   0,0,255 }; //blue


//   ___  ___ _ __  _ __ ___  _ __ ___  
//  / _ \/ _ \ '_ \| '__/ _ \| '_ ` _ \ 
// |  __/  __/ |_) | | | (_) | | | | | |
//  \___|\___| .__/|_|  \___/|_| |_| |_|
//           |_|                        

struct Config { // this is the main config struct that holds everything we want to save/load from EEPROM
  uint16_t version;
  bool navlights;
  bool enabledShows[NUM_SHOWS_WITH_ALTITUDE];
} config;

void loadConfig() { // loads existing config from EEPROM, or if wrong version, sets up new defaults and saves them
  EEPROM.get(CONFIG_START, config);
  Serial.println(F("Loading config..."));
  if (config.version != CONFIG_VERSION) { // check if EEPROM version matches this code's version. re-initialize EEPROM if not matching
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
    Right.stopPoint(WING_LEDS - WING_NAV_LEDS);
    Left.stopPoint(WING_LEDS - WING_NAV_LEDS);
    Serial.println(F("on."));
  } else {
    Right.stopPoint(WING_LEDS);
    Left.stopPoint(WING_LEDS);
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

  if (bmp.begin(0x76)) { // initialize the altitude pressure sensor with I2C address 0x76
    Serial.println(F("BMP280 module found."));
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X4,       /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_63);  /* Standby time. */

    basePressure = bmp.readPressure()/100; // this gets the current pressure at "ground level," so we can get relative altitude

    Serial.print(F("Base Pressure: "));
    Serial.println(basePressure);
  } else { // no BMP280 module installed
    Serial.println(F("No BMP280 module found. Disabling altitude() function."));
    NUM_SHOWS = NUM_SHOWS_WITH_ALTITUDE - 1;
  }

  pinMode(PROGRAM_CYCLE_BTN, INPUT_PULLUP);
  pinMode(PROGRAM_ENABLE_BTN, INPUT_PULLUP);
  pinMode(RC_PIN1, INPUT);
  pinMode(RC_PIN2, INPUT);
  FastLED.addLeds<NEOPIXEL, RIGHT_PIN>(Right.leds, WING_LEDS);
  FastLED.addLeds<NEOPIXEL, LEFT_PIN>(Left.leds, WING_LEDS);
  FastLED.addLeds<NEOPIXEL, FUSE_PIN>(Fuse.leds, FUSE_LEDS);
  FastLED.addLeds<NEOPIXEL, NOSE_PIN>(Nose.leds, NOSE_LEDS);
  FastLED.addLeds<NEOPIXEL, TAIL_PIN>(Tail.leds, TAIL_LEDS);
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
  static unsigned long prevAutoMillis = 0;
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

  if (programMode) { // we are in program mode where the user can enable/disable programs
    // Are we exiting program mode?
    if (digitalRead(PROGRAM_CYCLE_BTN) == LOW) { // we're in program mode. is the Program/Cycle button pressed?
      programModeCounter = programModeCounter + (currentMillis - progMillis); // increment the counter by how many milliseconds have passed
      if (programModeCounter > 3000) { // Has the button been held down for 3 seconds?
        programMode = false;
        Serial.println(F("Exiting program mode"));
        // store current program values into eeprom
        saveConfig();
        programModeCounter = 0;
        programInit('w'); //strobe the leds to indicate leaving program mode
      }
    } else { // button not pressed
      if (programModeCounter > 0 && programModeCounter < 1000) { // a momentary press to cycle to the next program
        currentShow++;
        if (currentShow == NUM_SHOWS) {currentShow = 0;}
      }
      programModeCounter = 0;
    }
    
    if (digitalRead(PROGRAM_ENABLE_BTN) == LOW) { // we're in program mode. is the Program Enable/Disable button pressed?
      enableCounter = enableCounter + (currentMillis - progMillis); // increment the counter by how many milliseconds have passed
    } else { // button not pressed
      if (enableCounter > 0 && enableCounter < 1000) { // momentary press to toggle the current show
        // toggle the state of the current program on/off
        if (config.enabledShows[currentShow] == true) {config.enabledShows[currentShow] = false;}
        else {config.enabledShows[currentShow] = true;}
        programInit(config.enabledShows[currentShow]);
      }
      enableCounter = 0;
    }

  } else { // we are not in program mode. Read signal from receiver and run through programs normally.

    if (rcInputPort == 0 || rcInputPort == 1) { // if rcInputPort == 0, check both rc input pins until we get a valid signal on one
      currentCh1 = pulseIn(RC_PIN1, HIGH, 25000);  // (Pin, State, Timeout)
      if (currentCh1 > 700 && currentCh1 < 2400) { // do we have a valid signal?
        if (rcInputPort == 0) {rcInputPort = 1;} // if we were on "either" port mode, switch it to 1
        currentShow = map(currentCh1, 950, 1980, 0, numActiveShows-1); // mapping 950us - 1980us  to 0 - (numActiveShows-1). might still need timing tweaks.
      }
    }
    if (rcInputPort == 0 || rcInputPort == 2) { // RC_PIN2 is our 2-position-switch autoscroll mode
      currentCh2 = pulseIn(RC_PIN2, HIGH, 25000);  // (Pin, State, Timeout)
      if (currentCh2 > 700 && currentCh2 < 2400) { // valid signal?
        if (rcInputPort == 0) {rcInputPort = 2;} // if we were on "either" port mode, switch it to 2
        if (currentCh2 > 1500) {
          // switch is "up" (above 1500), auto-scroll through shows
          if (currentMillis - prevAutoMillis > 2000) { // auto-advance after 2 seconds
            currentShow += 1;
            prevAutoMillis = currentMillis;
          }
        } else { // switch is "down" (below 1500), stop autoscrolling, reset timer
          prevAutoMillis = currentMillis - 1995; // this keeps the the auto-advance timer constantly "primed", so when flipping the switch again, it advances right away
        }
      }
    }
    currentShow = currentShow % numActiveShows; // keep currentShow within the limits of our active shows
    
    // Are we entering program mode?
    if (digitalRead(PROGRAM_CYCLE_BTN) == LOW) { // Is the Program button pressed?
      programModeCounter = programModeCounter + (currentMillis - progMillis); // increment the counter by how many milliseconds have passed
      if (programModeCounter > 3000) { // Has the button been held down for 3 seconds?
        programMode = true;
        programModeCounter = 0;
        Serial.println(F("Entering program mode"));
        programInit('w'); //strobe the leds to indicate entering program mode
        currentShow = 0;
        programInit(config.enabledShows[currentShow]);
      }
    } else if (digitalRead(PROGRAM_ENABLE_BTN) == LOW) { // Program button not pressed, but is Enable/Disable button pressed?
      programModeCounter = programModeCounter + (currentMillis - progMillis);
      if (programModeCounter > 3000) {
        // toggle the navlights on/off
        if (config.navlights == true) {config.navlights = false;}
        else {config.navlights = true;}
        saveConfig();
        programModeCounter = 0;
      }
    } else { // no buttons are being pressed
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

void stepShow() { // this is the main "show rendering" update function. this plays the next "frame" of the current show
  if (currentShow != prevShow) { // did we just switch to a new show?
    Serial.print(F("Current Show: "));
    Serial.println(currentShow);
    currentStep = 0; // reset the global general-purpose counter
    blank();
    if (programMode) { // if we're in program mode and just switched, indicate show status
      programInit(config.enabledShows[currentShow]); //flash all LEDs red/green to indicate current show status
    }
  }

  int switchShow; // actual show to select in the switchcase
  if (programMode) { // if we're in program mode, select from all shows
    switchShow = currentShow;
  } else { // if not in program mode, only select from enabled shows
    switchShow = activeShowNumbers[currentShow];
  }

  switch (switchShow) { // activeShowNumbers[] will look like {1, 4, 5, 9}, so this maps to actual show numbers
    caseshow(0,  blank()); // all off except for NAV lights, if enabled
    caseshow(1,  colorWave1(10, 10));// regular rainbow
    caseshow(2,  colorWave1(0, 10)); // whole plane solid color rainbow
    caseshow(3,  setColor(CRGB::Red)); // whole plane solid color
    caseshow(4,  setColor(CRGB::Orange));
    caseshow(5,  setColor(CRGB::Yellow));
    caseshow(6,  setColor(CRGB::Green));
    caseshow(7,  setColor(CRGB::Blue));
    caseshow(8,  setColor(CRGB::Indigo));
    caseshow(9,  setColor(CRGB::DarkCyan));
    caseshow(10, setColor(CRGB::White));
    caseshow(11, twinkle1()); // twinkle effect
    caseshow(12, strobe(3)); // Realistic double strobe alternating between wings
    caseshow(13, strobe(2)); // Realistic landing-light style alternating between wings
    caseshow(14, strobe(1)); // unrealistic rapid strobe of all non-nav leds, good locator/identifier. also might cause seizures
    caseshow(15, chase(CRGB::White, CRGB::Black, 50, 35, 80));
    caseshow(16, cylon(CRGB::Red, CRGB::Black, 30, 30, 50)); // Night Rider/Cylon style red beam scanning back and forth
    caseshow(17, juggle(4, 8));
    caseshow(18, animateColor(USA, 4, 1));
    //altitude needs to be the last show so we can disable it if no BMP280 module is installed
    caseshow(19, altitude(0, variometer)); // first parameter is for testing. 0 for real live data, set to another number for "fake" altitude
  }
  prevShow = currentShow;
}

//   _          _                    __                  _   _                  
//  | |__   ___| |_ __   ___ _ __   / _|_   _ _ __   ___| |_(_) ___  _ __  ___  
//  | '_ \ / _ \ | '_ \ / _ \ '__| | |_| | | | '_ \ / __| __| |/ _ \| '_ \/ __| 
//  | | | |  __/ | |_) |  __/ |    |  _| |_| | | | | (__| |_| | (_) | | | \__ \ 
//  |_| |_|\___|_| .__/ \___|_|    |_|  \__,_|_| |_|\___|\__|_|\___/|_| |_|___/ 
//               |_|                                                            

void showStrip () { // wrapper for FastLED.show()
  #ifdef TMP_BRIGHTNESS
  FastLED.setBrightness(TMP_BRIGHTNESS);
  #endif
  FastLED.show();
}

void blank() { // Turn off all LEDs
  setColor(CRGB::Black);
  interval = 50;
  showStrip();
}

void setColor(CRGB color) { // sets all LEDs to a solid color
  fill_solid(Right.leds, wingNavPoint, color);
  fill_solid(Left.leds, wingNavPoint, color);
  fill_solid(Nose.leds, NOSE_LEDS, color);
  fill_solid(Fuse.leds, FUSE_LEDS, color);
  fill_solid(Tail.leds, TAIL_LEDS, color);
  interval = 50;
  showStrip();
}

void setColor (CRGBPalette16 palette) { // spreads a palette across all LEDs
  for (int i = 0; i < maxLeds; i++) {
    Right.set(i, ColorFromPalette(palette, map(i, 0, wingNavPoint, 0, 240)));
    Left.set(i, ColorFromPalette(palette, map(i, 0, wingNavPoint, 0, 240)));
    Tail.set(i, ColorFromPalette(palette, map(i, 0, TAIL_LEDS, 0, 240)));
    if (NOSE_FUSE_JOINED) {
      setNoseFuse(i, ColorFromPalette(palette, map(i, 0, NOSE_LEDS+FUSE_LEDS, 0, 240)));
    } else {
      Nose.set(i, ColorFromPalette(palette, map(i, 0, NOSE_LEDS, 0, 240)));
      Fuse.set(i, ColorFromPalette(palette, map(i, 0, FUSE_LEDS, 0, 240)));
    }
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
    case 'o': color = CRGB::Black; // o = off
              break;
  }
  return color;
}

void setPattern (char pattern[]) { // sets wings to a static pattern
  for (int i = 0; i < maxLeds; i++) {
    Right.set(i, LetterToColor(pattern[i]));
    Left.set(i, LetterToColor(pattern[i]));
  }
  interval = 20;
  showStrip();
}

void setInitPattern () { // set all LEDs to the static init pattern
  for (int i = 0; i < WING_LEDS; i++) {
    Right.set(i, LetterToColor(init_rightwing[i]));
  }
  
  for (int i = 0; i < WING_LEDS; i++) {
    Left.set(i, LetterToColor(init_leftwing[i]));
  }
  
  for (int i = 0; i < NOSE_LEDS; i++) {
    Nose.set(i, LetterToColor(init_nose[i]));
  }
  
  for (int i = 0; i < FUSE_LEDS; i++) {
    Fuse.set(i, LetterToColor(init_fuse[i]));
  }
  
  for (int i = 0; i < TAIL_LEDS; i++) {
    Tail.set(i, LetterToColor(init_tail[i]));
  }
  
  showStrip();
}

void animateColor (CRGBPalette16 palette, int ledOffset, int stepSize) { // animates a palette across all LEDs
  if (currentStep > 255) {currentStep -= 255;}
  for (uint8_t i = 0; i < maxLeds; i++) {
    int j = triwave8((i * ledOffset) + currentStep);
    CRGB color = ColorFromPalette(palette, scale8(j, 240));
    Right.set(i, color);
    Left.set(i, color);
    Tail.set(i, color);
    if (NOSE_FUSE_JOINED) {
      setNoseFuse(i, color);
    } else {
      Nose.set(i, color);
      Fuse.set(i, color);
    }
  }

  currentStep += stepSize;
  interval = 20;
  showStrip();
}

void setNoseFuse(uint8_t led, CRGB color) { setNoseFuse(led, color, false); } // overload for simple setting of leds
void setNoseFuse(uint8_t led, CRGB color, bool combine) { // sets leds along nose and fuse as if they were the same strip. range is 0 - ((NOSE_LEDS+FUSE_LEDS)-1). combine = true to combine the new led color with the old value
  if (led < NOSE_LEDS) {
    if (combine) {
      Nose.addor(led, color);
    } else {
      Nose.set(led, color);
    }
  } else {
    if (combine) {
      Fuse.addor(led-NOSE_LEDS, color);
    } else {
      Fuse.set(led-NOSE_LEDS, color);
    }
  }
}

void setBothWings(uint8_t led, CRGB color) { setBothWings(led, color, false); }// overload for simple setting of leds
void setBothWings(uint8_t led, CRGB color, bool combine) { // sets leds along both wings as if they were the same strip. range is 0 - ((wingNavPoint*2)-1). left wingNavPoint = 0, right wingNavPoint = max. combine = true to combine the new led color with the old value
  if (led < wingNavPoint) {
    if (combine) {
      Left.addor(wingNavPoint - led - 1, color);
    } else {
      Left.set(wingNavPoint - led - 1, color);
    }
  } else {
    if (combine) {
      Right.addor(led - wingNavPoint, color);
    } else {
      Right.set(led - wingNavPoint, color);
    }
  }
}

//               _                 _   _                  
//    __ _ _ __ (_)_ __ ___   __ _| |_(_) ___  _ __  ___  
//   / _` | '_ \| | '_ ` _ \ / _` | __| |/ _ \| '_ \/ __| 
//  | (_| | | | | | | | | | | (_| | |_| | (_) | | | \__ \ 
//   \__,_|_| |_|_|_| |_| |_|\__,_|\__|_|\___/|_| |_|___/ 

void colorWave1 (uint8_t ledOffset, uint8_t l_interval) { // Rainbow pattern
  if (currentStep > 255) {currentStep = 0;}
  for (uint8_t i = 0; i < maxLeds; i++) {
    CRGB color = CHSV(currentStep + (ledOffset * i), 255, 255);
    setBothWings(i, color);
    Tail.set(i, color);
    if (NOSE_FUSE_JOINED) {
      setNoseFuse(i, color);
    } else {
      Nose.set(i, color);
      Fuse.set(i, color);
    }
  }
  currentStep++;
  interval = l_interval;
  showStrip();
}

void chase(CRGB color1, CRGB color2, uint8_t speedWing, uint8_t speedFuse, uint8_t speedTail) { // overload to do a chase pattern
  chase(color1, color2, speedWing, speedFuse, speedTail, false);
}

void cylon(CRGB color1, CRGB color2, uint8_t speedWing, uint8_t speedFuse, uint8_t speedTail) { // overload to do a cylon pattern
  chase(color1, color2, speedWing, speedFuse, speedTail, true);
}

void chase(CRGB color1, CRGB color2, uint8_t speedWing, uint8_t speedFuse, uint8_t speedTail, bool cylon) { // main chase function. can do either chase or cylon patterns
  if (color2 == (CRGB)CRGB::Black) {
    Right.nscale8(192);
    Left.nscale8(192);
    Fuse.nscale8(192);
    Nose.nscale8(192);
    Tail.nscale8(192);
  } else {
    Right.lerp8(color2, 20);
    Left.lerp8(color2, 20);
    Fuse.lerp8(color2, 20);
    Nose.lerp8(color2, 20);
    Tail.lerp8(color2, 20);
  }

  if (cylon == true) {
    setBothWings(scale8(triwave8(beat8(speedWing)), (wingNavPoint*2)-1), color1);
    Tail.set(scale8(triwave8(beat8(speedTail)), TAIL_LEDS-1), color1);
    if (NOSE_FUSE_JOINED) {
      setNoseFuse(scale8(triwave8(beat8(speedFuse)), (NOSE_LEDS+FUSE_LEDS)-1), color1);
    } else {
      Nose.set(scale8(triwave8(beat8(speedFuse)), NOSE_LEDS-1), color1);
      Fuse.set(scale8(triwave8(beat8(speedFuse)), FUSE_LEDS-1), color1);
    }
  } else {
    Right.set(scale8(beat8(speedWing), wingNavPoint-1), color1);
    Left.set(scale8(beat8(speedWing), wingNavPoint-1), color1);
    Tail.set(scale8(beat8(speedTail), TAIL_LEDS-1), color1);
    if (NOSE_FUSE_JOINED) {
      setNoseFuse(scale8(beat8(speedFuse), (NOSE_LEDS+FUSE_LEDS)-1), color1);
    } else {
      Nose.set(scale8(beat8(speedFuse), NOSE_LEDS-1), color1);
      Fuse.set(scale8(beat8(speedFuse), FUSE_LEDS-1), color1);
    }
  }

  interval = 10;
  showStrip();
}

void juggle(uint8_t numPulses, uint8_t speed) { // a few "pulses" of light that bounce back and forth at different timings
  uint8_t spread = 256 / numPulses;

  Right.nscale8(192);
  Left.nscale8(192);
  Fuse.nscale8(192);
  Nose.nscale8(192);
  Tail.nscale8(192);

  for (uint8_t i = 0; i < numPulses; i++) {
    setBothWings(beatsin8(i+speed, 0, (wingNavPoint*2)-1), CHSV(i*spread + beat8(1), 200, 255), true); // setBothWings(..., true) does led[i] |= color, so colors add when overlapping
    Tail.addor(beatsin8(i+speed, 0, TAIL_LEDS-1), CHSV(i*spread + beat8(1), 200, 255));
    if (NOSE_FUSE_JOINED) {
      setNoseFuse(beatsin8(i+speed, 0, (NOSE_LEDS+FUSE_LEDS)-1), CHSV(i*spread + beat8(1), 200, 255), true);
    } else {
      Nose.addor(beatsin8(i+speed, 0, NOSE_LEDS-1), CHSV(i*spread + beat8(1), 200, 255));
      Fuse.addor(beatsin8(i+speed, 0, FUSE_LEDS-1), CHSV(i*spread + beat8(1), 200, 255));
    }
  }

  interval = 10;
  showStrip();
}

void setNavLeds(const struct CRGB& lcolor, const struct CRGB& rcolor) { // helper function for the nav lights
  for (uint8_t i = wingNavPoint; i < WING_LEDS; i++) {
    Left.set(i, lcolor);
    Right.set(i, rcolor);
  }
}

void navLights() { // persistent nav lights
static uint8_t navStrobeState = 0;
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
void strobe(int style) { // Various strobe patterns
  static bool StrobeState = true;

  switch(style) {
    case 1: // Rapid strobing all LEDS in unison
      switch(currentStep) {
        case 0:
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::White);
            Left.set(i, CRGB::White);
            Nose.set(i, CRGB::White);
            Fuse.set(i, CRGB::White);
            Tail.set(i, CRGB::White);
          }
        break;
        case 1:
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::Black);
            Left.set(i, CRGB::Black);
            Nose.set(i, CRGB::Black);
            Fuse.set(i, CRGB::Black);
            Tail.set(i, CRGB::Black);
          }
          currentStep = 0;
        break;
      }
    break;

    case 2: // Alternate strobing of left and right wing
      switch (currentStep) {
        case 0:
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::White);
            Left.set(i, CRGB::Black);
            Nose.set(i, CRGB::Blue);
            Fuse.set(i, CRGB::Blue);
            Tail.set(i, CRGB::White);
          }
        break;
        case 10:
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::Black);
            Left.set(i, CRGB::White);
            Nose.set(i, CRGB::Yellow);
            Fuse.set(i, CRGB::Yellow);
            Tail.set(i, CRGB::White);
          }
        break;
        case 19:
          currentStep = 0;
        break;
      }
    break;
    case 3: // alternate double-blink strobing of left and right wing
      switch(currentStep) {
        case 0: // Right wing on for 50ms
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::White);
            Left.set(i, CRGB::Black);
          }
        break;
        case 1: // Both wings off for 50ms
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::Black);
            Left.set(i, CRGB::Black);
          }
        break;
        case 2: // Right wing on for 50ms
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::White);
            Left.set(i, CRGB::Black);
          }
        break;
        case 3: // Both wings off for 500ms
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::Black);
            Left.set(i, CRGB::Black);
          }
        break;
        case 13: // Left wing on for 50ms
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::Black);
            Left.set(i, CRGB::White);
          }
        break;
        case 14: // Both wings off for 50ms
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::Black);
            Left.set(i, CRGB::Black);
          }
        break;
        case 15: // Left wing on for 50ms
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::Black);
            Left.set(i, CRGB::White);
          }
        break;
        case 16: // Both wings off for 500ms
          for (uint8_t i = 0; i < maxLeds; i++) {
            Right.set(i, CRGB::Black);
            Left.set(i, CRGB::Black);
          }
        break;
        case 25:
          currentStep = 0;
        break;
    }
    break;
  }

  // switch(style) {

  //   case 1: //Rapid strobing all LEDS in unison
  //     if (StrobeState) {
  //       for (int i = 0; i < wingNavPoint; i++) {
  //         Right.set(i, CRGB::White);
  //         Left.set(i, CRGB::White);
  //       }
  //       for (int i = 0; i < NOSE_LEDS; i++) {Nose.set(i, CRGB::White);}
  //       for (int i = 0; i < FUSE_LEDS; i++) {Fuse.set(i, CRGB::White);}
  //       for (int i = 0; i < TAIL_LEDS; i++) {Tail.set(i, CRGB::White);}
  //       StrobeState = false;
  //     } else {
  //       for (int i = 0; i < wingNavPoint; i++) {
  //         Right.set(i, CRGB::Black);
  //         Left.set(i, CRGB::Black);
  //       }
  //       for (int i = 0; i < NOSE_LEDS; i++) {Nose.set(i, CRGB::Black);}
  //       for (int i = 0; i < FUSE_LEDS; i++) {Fuse.set(i, CRGB::Black);}
  //       for (int i = 0; i < TAIL_LEDS; i++) {Tail.set(i, CRGB::Black);}
  //       StrobeState = true;
  //       }
  //     interval = 50;
  //     showStrip();
  //   break;

  //   case 2: //Alternate strobing of left and right wing
  //     if (StrobeState) {
  //       for (int i = 0; i < wingNavPoint; i++) {
  //         Right.set(i, CRGB::White);
  //         Left.set(i, CRGB::Black);
  //       }
  //       for (int i = 0; i < NOSE_LEDS; i++) {Nose.set(i, CRGB::Blue);}
  //       for (int i = 0; i < FUSE_LEDS; i++) {Fuse.set(i, CRGB::Blue);}
  //       for (int i = 0; i < TAIL_LEDS; i++) {Tail.set(i, CRGB::White);}
  //     } else {
  //       for (int i = 0; i < wingNavPoint; i++) {
  //         Right.set(i, CRGB::Black);
  //         Left.set(i, CRGB::White);
  //       }
  //       for (int i = 0; i < NOSE_LEDS; i++) {Nose.set(i, CRGB::Yellow);}
  //       for (int i = 0; i < FUSE_LEDS; i++) {Fuse.set(i, CRGB::Yellow);}
  //       for (int i = 0; i < TAIL_LEDS; i++) {Tail.set(i, CRGB::White);}
  //     }
  //     interval = 500;
  //     StrobeState = !StrobeState;
  //     showStrip();
  //   break;

  //   case 3: //alternate double-blink strobing of left and right wing

  //     switch(currentStep) {

  //       case 0: // Right wing on for 50ms
  //         for (int i = 0; i < wingNavPoint; i++) {
  //           Right.set(i, CRGB::White);
  //           Left.set(i, CRGB::Black);
  //         }
  //         interval = 50;
  //       break;
          
  //       case 1: // Both wings off for 50ms
  //         for (int i = 0; i < wingNavPoint; i++) {
  //           Right.set(i, CRGB::Black);
  //           Left.set(i, CRGB::Black);
  //         }
  //         interval = 50;
  //       break;
          
  //       case 2: // Right wing on for 50ms
  //         for (int i = 0; i < wingNavPoint; i++) {
  //           Right.set(i, CRGB::White);
  //           Left.set(i, CRGB::Black);
  //         }
  //         interval = 50;
  //       break;
          
  //       case 3: // Both wings off for 500ms
  //         for (int i = 0; i < wingNavPoint; i++) {
  //           Right.set(i, CRGB::Black);
  //           Left.set(i, CRGB::Black);
  //         }
  //         interval = 500;
  //       break;
          
  //       case 4: // Left wing on for 50ms
  //         for (int i = 0; i < wingNavPoint; i++) {
  //           Right.set(i, CRGB::Black);
  //           Left.set(i, CRGB::White);
  //         }
  //         interval = 50;
  //       break;
          
  //       case 5: // Both wings off for 50ms
  //         for (int i = 0; i < wingNavPoint; i++) {
  //           Right.set(i, CRGB::Black);
  //           Left.set(i, CRGB::Black);
  //         }
  //         interval = 50;
  //       break;
          
  //       case 6: // Left wing on for 50ms
  //         for (int i = 0; i < wingNavPoint; i++) {
  //           Right.set(i, CRGB::Black);
  //           Left.set(i, CRGB::White);
  //         }
  //         interval = 50;
  //       break;
          
  //       case 7: // Both wings off for 500ms
  //         for (int i = 0; i < wingNavPoint; i++) {
  //           Right.set(i, CRGB::Black);
  //           Left.set(i, CRGB::Black);
  //         }
  //         interval = 500;
  //       break;
                    
  //     }

  //     showStrip();
  //     currentStep++;
  //     if (currentStep == 8) {currentStep = 0;}
  //   break;

  // }

  interval = 50;
  currentStep++;
  showStrip();
}

void altitude(double fake, CRGBPalette16 palette) { // Altitude indicator show. wings fill up to indicate altitude, tail goes green/red as variometer
  static double prevAlt;
  static int avgVSpeed[] = {0,0,0,0};

  interval = 100; // we're also going to use interval as a "time delta" to base the vspeed off of

  int vSpeed;
  double currentAlt;

  currentAlt = bmp.readAltitude(basePressure)*METRIC_CONVERSION;
  
  if (fake != 0) {currentAlt = fake;}

  //Rewrite of the altitude LED graph. Wings and Fuse all graphically indicate relative altitude AGL from zero to MAX_ALTIMETER
  if (currentAlt > MAX_ALTIMETER) {currentAlt = MAX_ALTIMETER;}
  
  for (int i=map(currentAlt, 0, MAX_ALTIMETER, 0, wingNavPoint); i < wingNavPoint; i++) {
    Right.set(i, CRGB::Black);
    Left.set(i, CRGB::Black);
  }
  for (int i=map(currentAlt, 0, MAX_ALTIMETER, 0, FUSE_LEDS); i < FUSE_LEDS; i++) {
    Fuse.set(i, CRGB::Black);
  }
  for (int i=0; i < map(currentAlt, 0, MAX_ALTIMETER, 0, wingNavPoint); i++) {
    if ((i % 2) == 0) {
      Right.set(i, CRGB::White);
      Left.set(i, CRGB::White);
    } else {
      Right.set(i, CRGB::Green);
      Left.set(i, CRGB::Green);
    }
  }
  for (int i=0; i < map(currentAlt, 0, MAX_ALTIMETER, 0, FUSE_LEDS); i++) {
    if ((i % 2) == 0) {
      Fuse.set(i, CRGB::White);
    } else {
      Fuse.set(i, CRGB::Green);
    }
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
    Tail.set(i, ColorFromPalette(palette, vspeedMap));
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
void doTwinkle1(struct CRGB * ledArray, uint8_t * pixelState, uint8_t size) { // helper function for the twinkle show
  const CRGB colorDown = CRGB(1, 1, 1);
  const CRGB colorUp = CRGB(8, 8, 8);
  const CRGB colorMax = CRGB(128, 128, 128);
  const CRGB colorMin = CRGB(0, 0, 0);
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

  doTwinkle1(Right.leds, pixelStateRight, wingNavPoint);
  doTwinkle1(Left.leds,  pixelStateLeft, wingNavPoint);
  doTwinkle1(Nose.leds,  pixelStateNose, NOSE_LEDS);
  doTwinkle1(Fuse.leds,  pixelStateFuse, FUSE_LEDS);
  doTwinkle1(Tail.leds,  pixelStateTail, TAIL_LEDS);

  interval = 10;
  showStrip();
}

void programInit(bool progState) { // overload, takes a bool and flashes red/green depending on true or not
  if (progState) {
    Serial.println(F("enabled."));
    programInit('g');
  } else {
    Serial.println(F("disabled."));
    programInit('r');
  }
}

void programInit(char progState) { // flashes red/green/white for different program mode indicators
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
  for (int j = 0; j < 7; j++) {
    for (int i = 0; i < maxLeds; i++) {
      Right.set(i, color);
      Left.set(i, color);
      Nose.set(i, color);
      Fuse.set(i, color);
      Tail.set(i, color);
    }
    digitalWrite(LED_BUILTIN, HIGH);
    showStrip();
    delay(50);
    for (int i = 0; i < maxLeds; i++) {
      Right.set(i, CRGB::Black);
      Left.set(i, CRGB::Black);
      Nose.set(i, CRGB::Black);
      Fuse.set(i, CRGB::Black);
      Tail.set(i, CRGB::Black);
    }
    digitalWrite(LED_BUILTIN, LOW);
    showStrip();
    delay(50);
  }
  digitalWrite(LED_BUILTIN, LOW);
}
