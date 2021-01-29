//
// ---- FT Night Radian LED Controller ----
//
// VERSION: 1.2.2
// DATE:    2020-06-07
//
#define FASTLED_INTERNAL // disables the FastLED version message that looks like an error

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

#define CONFIG_VERSION 0xAA03 // EEPROM config version (increment this any time the Config struct or number of shows changes).
#define CONFIG_START 0 // starting EEPROM address for our config

#define METRIC_CONVERSION 3.3; // 3.3 to convert meters to feet. 1 for meters.

#define caseshow(x,y) case x: y; break; // macro for switchcases with a built-in break

uint8_t NUM_SHOWS = NUM_SHOWS_WITH_ALTITUDE; // NUM_SHOWS becomes 1 less if no BMP280 module is installed

const uint8_t maxLeds = max(WING_LEDS, max((NOSE_LEDS+FUSE_LEDS), TAIL_LEDS));

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
bool hasBMP280 = false; // did we detect a BMP280 module?

// Class: LED
// ----------------------------
//   the main class for the different LED strip objects
//   this mainly functions as a "wrapper" for the FastLED arrays, with some helpful methods built-in
//
//   leds: a pointer to the real FastLED array that we pass in
//   reversed: whether this strip is "reversed" from normal operation
//   numLeds: the physical number of LEDs in the strip (and array)
//   stopPoint: this is where the normal set() function will stop. this is mainly useful for navlights on the wings
class LED {
public:
  CRGB* leds;
  bool reversed;
  uint8_t numLeds;
  uint8_t stopPoint;

  // Function: LED (class constructor)
  // ---------------------------------
  //   runs when class object is initialized
  //
  //   *ledArray: pointer to the actual FastLED array this object uses
  //   num: size of the ledArray
  //   rev: weather this LED string is "reversed" from normal operation or not
  LED(CRGB * ledarray, uint8_t num, bool rev) {
    reversed = rev;
    numLeds = num;
    stopPoint = num;
    leds = ledarray; // Sets the internal 'leds' pointer to point to the "real" led array
  }

  // Function: set
  // -------------
  //   sets LEDs in this object to the specified color
  //   also handles reversing and "out-of-bounds" checking
  //
  //   led: which LED in the array to modify
  //   color: a CRGB color to set the LED to
  void set(uint8_t led, const CRGB& color) {
    if (led < stopPoint) {
      if (reversed) {
        leds[numLeds - led - 1] = color;
      } else {
        leds[led] = color;
      }
    }
  }

  // Function: setNav
  // ----------------
  //   sets navlight section of this object to specified color
  //
  //   color: a CRGB color to set the navlights to
  void setNav(const CRGB& color) {
    for (uint8_t i = 0; i < WING_NAV_LEDS; i++) {
      if (reversed) { // if reversed, start at the "beginning" of the led string
        leds[i] = color;
      } else { // not reversed, so start at the "end" of the string and work inwards.
        leds[numLeds - i - 1] = color;
      }
    }
  }

  // Function: add
  // -------------
  //   adds color to existing value
  //
  //   led: which LED in the array to modify
  //   color: a CRGB color to set the LED to
  void add(uint8_t led, const CRGB& color) {
    if (led < stopPoint) {
      if (reversed) {
        leds[numLeds - led - 1] += color;
      } else {
        leds[led] += color;
      }
    }
  }

  // Function: addor
  // ---------------
  //   "or"s the colors, making the LED the brighter of the two
  //
  //   led: which LED in the array to modify
  //   color: a CRGB color to set the LED to
  void addor(uint8_t led, const CRGB& color) {
    if (led < stopPoint) {
      if (reversed) {
        leds[numLeds - led - 1] |= color;
      } else {
        leds[led] += color;
      }
    }
  }

  // Function: nscale8
  // -----------------
  //   fades the whole string down by the specified scale
  //   used when needing to fade a "trail" to black
  //
  //   scale: value from 0-255 to scale the existing colors by
  void nscale8(uint8_t scale) {
    for (uint8_t i = 0; i < stopPoint; i++) {
      if (reversed) {
        leds[numLeds - i - 1] = leds[numLeds - i - 1].nscale8(scale);
      } else {
        leds[i] = leds[i].nscale8(scale);
      }
    }
  }

  // Function: lerp8
  // ---------------
  //   interpolates between existing LED color and specified color
  //
  //   other: new color to interpolate towards
  //   frac: value from 0-255 that specifies how much interpolation happens
  void lerp8(const CRGB& other, uint8_t frac) {
    for (uint8_t i = 0; i < stopPoint; i++) {
      if (reversed) {
        leds[numLeds - i - 1] = leds[numLeds - i - 1].lerp8(other, frac);
      } else {
        leds[i] = leds[i].lerp8(other, frac);
      }
    }
  }
};

// set up the FastLED array variables
CRGB rightleds[WING_LEDS];
CRGB leftleds[WING_LEDS];
CRGB noseleds[NOSE_LEDS];
CRGB fuseleds[FUSE_LEDS];
CRGB tailleds[TAIL_LEDS];

// initialize the LED class objects for each string,
// passing in the actual array, the length of each string,
// and if they're reversed or not
LED Right(rightleds, WING_LEDS, WING_REV);
LED Left(leftleds, WING_LEDS, WING_REV);
LED Nose(noseleds, NOSE_LEDS, NOSE_REV);
LED Fuse(fuseleds, FUSE_LEDS, FUSE_REV);
LED Tail(tailleds, TAIL_LEDS, TAIL_REV);

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

// Struct: Config
// --------------
//   this is the main config struct that holds everything we want to save/load from EEPROM
struct Config {
  uint16_t version; // what version this config struct is
  bool navlights; // whether navlights are enabled/disabled
  bool enabledShows[NUM_SHOWS_WITH_ALTITUDE]; // which shows are active/disabled
} config;

// Function: loadConfig
// --------------------
//   loads existing config from EEPROM, or if version mismatch, sets up new defaults and saves them
void loadConfig() {
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

// Function: saveConfig
// --------------------
//   saves current config to EEPROM
void saveConfig() { // 
  EEPROM.put(CONFIG_START, config);
  Serial.println(F("Saving config..."));
  updateShowConfig();
}

// Function: updateShowConfig
// --------------------------
//   sets order of currently active shows. e.g., activeShowNumbers[] = {1, 4, 5, 9}
//   also sets nav stop point
void updateShowConfig() {
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
  if (config.navlights) { // navlights are on, set stopPoint to (total number) - (number of navlights)
    Right.stopPoint = WING_LEDS - WING_NAV_LEDS;
    Left.stopPoint = WING_LEDS - WING_NAV_LEDS;
    Serial.println(F("on."));
  } else { // navlights are off, set stopPoint to max number of LEDs
    Right.stopPoint = WING_LEDS;
    Left.stopPoint = WING_LEDS;
    Serial.println(F("off."));
  }
}

//            _                
//   ___  ___| |_ _   _ _ __   
//  / __|/ _ \ __| | | | '_ \  
//  \__ \  __/ |_| |_| | |_) | 
//  |___/\___|\__|\__,_| .__/  
//                     |_|     

// Function: setup
// ---------------
//   arduino first-run function
void setup() {
  Serial.begin(115200);

  loadConfig();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (bmp.begin(0x76)) { // initialize the altitude pressure sensor with I2C address 0x76
    hasBMP280 = true;
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

  // initialize FastLED arrays
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

// Function: loop
// --------------
//   arduino main loop
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
        statusFlash('w'); //strobe the leds to indicate leaving program mode
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
        if (config.enabledShows[currentShow] == true) {
          config.enabledShows[currentShow] = false;
          Serial.println(F("disabled"));
        } else {
          config.enabledShows[currentShow] = true;
          Serial.println(F("enabled"));
        }
        statusFlash(config.enabledShows[currentShow]);
      }
      enableCounter = 0;
    }

  } else { // we are not in program mode. Read signal from receiver and run through programs normally.
    if (rcInputPort == 0 || rcInputPort == 1) { // if rcInputPort == 0, check both rc input pins until we get a valid signal on one
      currentCh1 = pulseIn(RC_PIN1, HIGH, 25000);  // (Pin, State, Timeout)
      if (currentCh1 > 700 && currentCh1 < 2400) { // do we have a valid signal?
        if (rcInputPort == 0) {
          rcInputPort = 1; // if we were on "either" port mode, switch it to 1
          statusFlash('w', 1, 300); // flash white once for RC input 1
          statusFlash(hasBMP280, 1, 300); // indicate BMP280 module present
        }
        currentShow = map(currentCh1, 950, 1960, 0, numActiveShows-1); // mapping 950us - 1960us  to 0 - (numActiveShows-1). might still need timing tweaks.
      }
    }
    if (rcInputPort == 0 || rcInputPort == 2) { // RC_PIN2 is our 2-position-switch autoscroll mode
      currentCh2 = pulseIn(RC_PIN2, HIGH, 25000);  // (Pin, State, Timeout)
      if (currentCh2 > 700 && currentCh2 < 2400) { // valid signal?
        if (rcInputPort == 0) {
          rcInputPort = 2; // if we were on "either" port mode, switch it to 2
          statusFlash('w', 2, 300); // flash white twice for RC input 2
          statusFlash(hasBMP280, 1, 300); // indicate BMP280 module present
        }
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
    if (rcInputPort == 0) {
      statusFlash('r', 1, 300); // flash red to indicate no signal
    }
    currentShow = currentShow % numActiveShows; // keep currentShow within the limits of our active shows
    
    // Are we entering program mode?
    if (digitalRead(PROGRAM_CYCLE_BTN) == LOW) { // Is the Program button pressed?
      programModeCounter = programModeCounter + (currentMillis - progMillis); // increment the counter by how many milliseconds have passed
      if (programModeCounter > 3000) { // Has the button been held down for 3 seconds?
        programMode = true;
        programModeCounter = 0;
        Serial.println(F("Entering program mode"));
        statusFlash('w'); //strobe the leds to indicate entering program mode
        currentShow = 0;
        statusFlash(config.enabledShows[currentShow]);
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


// Function: stepShow
// ------------------
//   this is the main "show rendering" update function
//   this plays the next "frame" of the current show
void stepShow() {
  if (currentShow != prevShow) { // did we just switch to a new show?
    Serial.print(F("Current Show: "));
    Serial.println(currentShow);
    currentStep = 0; // reset the global general-purpose counter
    blank();
    if (programMode) { // if we're in program mode and just switched, indicate show status
      statusFlash(config.enabledShows[currentShow]); //flash all LEDs red/green to indicate current show status
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
    caseshow(1,  colorWave1(10, 10)); // regular rainbow
    caseshow(2,  colorWave1(0, 10)); // zero led offset, so the whole plane is a solid color rainbow
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
    caseshow(15, chase(CRGB::White, CRGB::Black, 50, 80, 35, 80)); // "chase" effect, with a white streak on a black background
    caseshow(16, cylon(CRGB::Red, CRGB::Black, 30, 50, 30, 50)); // Night Rider/Cylon style red beam scanning back and forth
    caseshow(17, juggle(4, 8)); // multiple unique "pulses" of light bouncing back and forth, all with different colors
    caseshow(18, animateColor(USA, 4, 1)); // sweeps a palette across the whole plane

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

// Function: showStrip
// -------------------
//   wrapper for FastLED.show()
//   this is in case we need to do anything extra (like set TMP_BRIGHTNESS) before actually calling FastLED.Show()
void showStrip () {
  #ifdef TMP_BRIGHTNESS
  FastLED.setBrightness(TMP_BRIGHTNESS);
  #endif
  FastLED.show();
}

// Function: blank
// ---------------
//   Turn off all LEDs
void blank() {
  setColor(CRGB::Black);
  interval = 50;
  showStrip();
}

// Function: setColor(CRGB)
// ------------------
//   sets all LEDs to a solid color
//
//   color: a CRGB color to set the LEDs to
void setColor(const CRGB& color) {
  fill_solid(Right.leds, Right.stopPoint, color);
  fill_solid(Left.leds, Left.stopPoint, color);
  fill_solid(Nose.leds, NOSE_LEDS, color);
  fill_solid(Fuse.leds, FUSE_LEDS, color);
  fill_solid(Tail.leds, TAIL_LEDS, color);
  interval = 50;
  showStrip();
}

// Function: setColor(CRGBPalette16)
// ------------------
//   spreads a palette across all LEDs
//
//   palette: pre-defined CRGBPalette16 object that will be used
void setColor (const CRGBPalette16& palette) {
  for (int i = 0; i < maxLeds; i++) {
    // range of 0-240 is used in the map() function due to how the FastLED ColorFromPalette() function works.
    // 240 is actually the correct "wrap-around" point
    Right.set(i, ColorFromPalette(palette, map(i, 0, Right.stopPoint, 0, 240)));
    Left.set(i, ColorFromPalette(palette, map(i, 0, Left.stopPoint, 0, 240)));
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

// Function: LetterToColor
// -----------------------
//   convert the letters in the static patterns to color values
//
//   letter: a single char string that will become an actual CRGB value
//
//   returns: a new CRGB color
CRGB LetterToColor (char letter) {
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

// Function: setPattern
// --------------------
//   sets wings to a static pattern
//
//   pattern: pre-defined static pattern array
void setPattern (char pattern[]) {
  for (int i = 0; i < maxLeds; i++) {
    Right.set(i, LetterToColor(pattern[i]));
    Left.set(i, LetterToColor(pattern[i]));
  }
  interval = 20;
  showStrip();
}

// Function: setInitPattern
// ------------------------
//   set all LEDs to the static init pattern
void setInitPattern () {
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

// Function: animateColor
// ----------------------
//   animates a palette across all LEDs
//
//   palette: pre-defined CRGBPalette16 object that will be used
//   ledOffset: how much each led is offset in the palette compared to the previous led
//   stepSize: how fast the leds will cycle through the palette
void animateColor (const CRGBPalette16& palette, int ledOffset, int stepSize) {
  if (currentStep > 255) {currentStep -= 256;}
  for (uint8_t i = 0; i < maxLeds; i++) {
    // scale to 240 again because that's the correct "wrap" point for ColorFromPalette()
    CRGB color = ColorFromPalette(palette, scale8(triwave8((i * ledOffset) + currentStep), 240));
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

// Function: setNoseFuse
// ---------------------
//   sets leds along nose and fuse as if they were the same strip
//   range is 0 - ((NOSE_LEDS+FUSE_LEDS)-1)
//
//   led: which LED to modify
//   color: a CRGB color to set the LED to
//   (optional) addor: set true to "or" the new led color with the old value. if not set, defaults to false
void setNoseFuse(uint8_t led, const CRGB& color) { setNoseFuse(led, color, false); } // overload for simple setting of leds
void setNoseFuse(uint8_t led, const CRGB& color, bool addor) {
  if (led < NOSE_LEDS) { // less than NOSE_LEDS, set the nose
    if (addor) {
      Nose.addor(led, color);
    } else {
      Nose.set(led, color);
    }
  } else { // greater than NOSE_LEDS, set fuse (minus NOSE_LEDS)
    if (addor) {
      Fuse.addor(led-NOSE_LEDS, color);
    } else {
      Fuse.set(led-NOSE_LEDS, color);
    }
  }
}

// Function: setBothWings
// ----------------------
//   sets leds along both wings as if they were the same strip
//   range is 0 - ((stopPoint*2)-1). left.stopPoint = 0, right.stopPoint = max
//
//   led: which LED to modify
//   color: a CRGB color to set the LED to
//   (optional) addor: set true to "or" the new led color with the old value. if not set, defaults to false
void setBothWings(uint8_t led, const CRGB& color) { setBothWings(led, color, false); } // overload for simple setting of leds
void setBothWings(uint8_t led, const CRGB& color, bool addor) {
  if (led < Left.stopPoint) { // less than left size, set left wing, but "reversed" (start at outside)
    if (addor) {
      Left.addor(Left.stopPoint - led - 1, color);
    } else {
      Left.set(Left.stopPoint - led - 1, color);
    }
  } else { // greater than left size, set right wing
    if (addor) {
      Right.addor(led - Left.stopPoint, color);
    } else {
      Right.set(led - Left.stopPoint, color);
    }
  }
}

//               _                 _   _                  
//    __ _ _ __ (_)_ __ ___   __ _| |_(_) ___  _ __  ___  
//   / _` | '_ \| | '_ ` _ \ / _` | __| |/ _ \| '_ \/ __| 
//  | (_| | | | | | | | | | | (_| | |_| | (_) | | | \__ \ 
//   \__,_|_| |_|_|_| |_| |_|\__,_|\__|_|\___/|_| |_|___/ 

// Function: colorWave1
// --------------------
//   rainbow pattern
//
//   ledOffset: how much each led is offset in the rainbow compared to the previous led
//   l_interval: sets interval for this show
void colorWave1 (uint8_t ledOffset, uint8_t l_interval) {
  if (currentStep > 255) {currentStep = 0;}
  for (uint8_t i = 0; i < Left.stopPoint + Right.stopPoint; i++) {
    setBothWings(i, CHSV(currentStep + (ledOffset * i), 255, 255));
  }
  for (uint8_t i = 0; i < maxLeds; i++) {
    // the CHSV() function uses uint8_t, so wrap-around is already taken care of
    CRGB color = CHSV(currentStep + (ledOffset * i), 255, 255);
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

// Function: chase/cylon
// ---------------------
//   LED chase functions with fadeout of the tail
//   can do more traditional same-direction chase, or back-and-forth "cylon/knight-rider" style
//
//   color1: "main" color of the chase effect
//   color2: "background" color
//   speedWing: chase speed of the wing leds
//   speedNose: chase speed of the nose leds
//   speedFuse: chase speed of the fuse leds
//   speedTail: chase speed of the tail leds
//   (optional) cylon: if this is set, does back-and-forth effect. regular chase otherwise. overloads set this.
void chase(const CRGB& color1, const CRGB& color2, uint8_t speedWing, uint8_t speedNose, uint8_t speedFuse, uint8_t speedTail) { // overload to do a chase pattern
  chase(color1, color2, speedWing, speedNose, speedFuse, speedTail, false);
}
// overload to do a cylon pattern
void cylon(const CRGB& color1, const CRGB& color2, uint8_t speedWing, uint8_t speedNose, uint8_t speedFuse, uint8_t speedTail) {
  chase(color1, color2, speedWing, speedNose, speedFuse, speedTail, true);
}
// main chase function. can do either chase or cylon patterns
void chase(const CRGB& color1, const CRGB& color2, uint8_t speedWing, uint8_t speedNose, uint8_t speedFuse, uint8_t speedTail, bool cylon) {
  // fade out the whole string to get a nice fading "trail"
  if (color2 == (CRGB)CRGB::Black) { // if our second color is black, do nscale8, because lerp never gets there
    Right.nscale8(192);
    Left.nscale8(192);
    Fuse.nscale8(192);
    Nose.nscale8(192);
    Tail.nscale8(192);
  } else { // otherwise, just lerp between the colors 
    Right.lerp8(color2, 20);
    Left.lerp8(color2, 20);
    Fuse.lerp8(color2, 20);
    Nose.lerp8(color2, 20);
    Tail.lerp8(color2, 20);
  }

  if (cylon == true) {
    // cylon uses both wings, and triwave to get a nice back-and-forth
    setBothWings(scale8(triwave8(beat8(speedWing)), (Right.stopPoint+Left.stopPoint)-1), color1);
    Tail.set(scale8(triwave8(beat8(speedTail)), TAIL_LEDS-1), color1);
    if (NOSE_FUSE_JOINED) {
      setNoseFuse(scale8(triwave8(beat8(speedFuse)), (NOSE_LEDS+FUSE_LEDS)-1), color1);
    } else {
      Nose.set(scale8(triwave8(beat8(speedNose)), NOSE_LEDS-1), color1);
      Fuse.set(scale8(triwave8(beat8(speedFuse)), FUSE_LEDS-1), color1);
    }
  } else {
    // chase just goes "out" in the same directions
    Right.set(scale8(beat8(speedWing), Right.stopPoint-1), color1);
    Left.set(scale8(beat8(speedWing), Left.stopPoint-1), color1);
    Tail.set(scale8(beat8(speedTail), TAIL_LEDS-1), color1);
    if (NOSE_FUSE_JOINED) {
      setNoseFuse(scale8(beat8(speedFuse), (NOSE_LEDS+FUSE_LEDS)-1), color1);
    } else {
      Nose.set(scale8(beat8(speedNose), NOSE_LEDS-1), color1);
      Fuse.set(scale8(beat8(speedFuse), FUSE_LEDS-1), color1);
    }
  }

  interval = 10;
  showStrip();
}

// Function: juggle
// ----------------
//   a few "pulses" of light that bounce back and forth at different timings
//
//   numPulses: how many unique pulses per string
//   speed: how fast the pulses move back and forth
void juggle(uint8_t numPulses, uint8_t speed) {
  uint8_t spread = 256 / numPulses;

  // fade out the whole string to get a nice fading "trail"
  Right.nscale8(192);
  Left.nscale8(192);
  Fuse.nscale8(192);
  Nose.nscale8(192);
  Tail.nscale8(192);

  for (uint8_t i = 0; i < numPulses; i++) {
    // use addor on everything, so colors add when overlapping
    setBothWings(beatsin8(i+speed, 0, (Right.stopPoint+Left.stopPoint)-1), CHSV(i*spread + beat8(1), 200, 255), true);
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

// Function: navLights
// -------------------
//   main function that animates the persistent navlights
void navLights() {
static uint8_t navStrobeState = 0;
  switch(navStrobeState) {
    case 0:
      // red/green
      Left.setNav(CRGB::Red);
      Right.setNav(CRGB::Green);
      break;
    case 50:
      // strobe 1
      Left.setNav(CRGB::White);
      Right.setNav(CRGB::White);
      break;
    case 52:
      // back to red/green
      Left.setNav(CRGB::Red);
      Right.setNav(CRGB::Green);
      break;
    case 54:
      // strobe 2
      Left.setNav(CRGB::White);
      Right.setNav(CRGB::White);
      break;
    case 56:
      // red/green again
      Left.setNav(CRGB::Red);
      Right.setNav(CRGB::Green);
      navStrobeState = 0;
      break;
  }
  showStrip();
  navStrobeState++;
}

// Function: strobe
// ----------------
//   various strobe patterns
//
//   style: which strobe style to use
//     1: rapid strobing all LEDS in unison. bad. might cause seizures
//     2: alternate strobing of left and right wing
//     3: alternate double-blink strobing of left and right wing
void strobe(int style) {
  switch(style) {
    case 1: // Rapid strobing all LEDS in unison
      switch(currentStep) {
        case 0:
            fill_solid(Right.leds, Right.stopPoint, CRGB::White);
            fill_solid(Left.leds, Left.stopPoint, CRGB::White);
            fill_solid(Nose.leds, NOSE_LEDS, CRGB::White);
            fill_solid(Fuse.leds, FUSE_LEDS, CRGB::White);
            fill_solid(Tail.leds, TAIL_LEDS, CRGB::White);
        break;
        case 1:
            fill_solid(Right.leds, Right.stopPoint, CRGB::Black);
            fill_solid(Left.leds, Left.stopPoint, CRGB::Black);
            fill_solid(Nose.leds, NOSE_LEDS, CRGB::Black);
            fill_solid(Fuse.leds, FUSE_LEDS, CRGB::Black);
            fill_solid(Tail.leds, TAIL_LEDS, CRGB::Black);
          currentStep = -1;
        break;
      }
    break;

    case 2: // Alternate strobing of left and right wing
      switch (currentStep) {
        case 0:
            fill_solid(Right.leds, Right.stopPoint, CRGB::White);
            fill_solid(Left.leds, Left.stopPoint, CRGB::Black);
            fill_solid(Nose.leds, NOSE_LEDS, CRGB::Blue);
            fill_solid(Fuse.leds, FUSE_LEDS, CRGB::Blue);
            fill_solid(Tail.leds, TAIL_LEDS, CRGB::White);
        break;
        case 10:
            fill_solid(Right.leds, Right.stopPoint, CRGB::Black);
            fill_solid(Left.leds, Left.stopPoint, CRGB::White);
            fill_solid(Nose.leds, NOSE_LEDS, CRGB::Yellow);
            fill_solid(Fuse.leds, FUSE_LEDS, CRGB::Yellow);
            fill_solid(Tail.leds, TAIL_LEDS, CRGB::White);
        break;
        case 19:
          currentStep = -1;
        break;
      }
    break;
    case 3: // alternate double-blink strobing of left and right wing
      switch(currentStep) {
        case 0: // Right wing on for 50ms
            fill_solid(Right.leds, Right.stopPoint, CRGB::White);
            fill_solid(Left.leds, Left.stopPoint, CRGB::Black);
        break;
        case 1: // Both wings off for 50ms
            fill_solid(Right.leds, Right.stopPoint, CRGB::Black);
            fill_solid(Left.leds, Left.stopPoint, CRGB::Black);
        break;
        case 2: // Right wing on for 50ms
            fill_solid(Right.leds, Right.stopPoint, CRGB::White);
            fill_solid(Left.leds, Left.stopPoint, CRGB::Black);
        break;
        case 3: // Both wings off for 500ms
            fill_solid(Right.leds, Right.stopPoint, CRGB::Black);
            fill_solid(Left.leds, Left.stopPoint, CRGB::Black);
        break;
        case 13: // Left wing on for 50ms
            fill_solid(Right.leds, Right.stopPoint, CRGB::Black);
            fill_solid(Left.leds, Left.stopPoint, CRGB::White);
        break;
        case 14: // Both wings off for 50ms
            fill_solid(Right.leds, Right.stopPoint, CRGB::Black);
            fill_solid(Left.leds, Left.stopPoint, CRGB::Black);
        break;
        case 15: // Left wing on for 50ms
            fill_solid(Right.leds, Right.stopPoint, CRGB::Black);
            fill_solid(Left.leds, Left.stopPoint, CRGB::White);
        break;
        case 16: // Both wings off for 500ms
            fill_solid(Right.leds, Right.stopPoint, CRGB::Black);
            fill_solid(Left.leds, Left.stopPoint, CRGB::Black);
        break;
        case 25:
          currentStep = -1;
        break;
    }
    break;
  }

  interval = 50;
  currentStep++;
  showStrip();
}

// Function: altitude
// ------------------
//   altitude indicator show
//   wings fill up to indicate altitude, tail goes green/red as variometer
//
//   fake: set to 0 for real data, anything else for testing
//   palette: gradient palette for the visual variometer on the tail
void altitude(double fake, const CRGBPalette16& palette) {
  static double prevAlt;
  static int avgVSpeed[] = {0,0,0,0};

  interval = 100; // we're also going to use interval as a "time delta" to base the vspeed off of

  int vSpeed;
  double currentAlt;

  currentAlt = bmp.readAltitude(basePressure)*METRIC_CONVERSION;
  
  if (fake != 0) {currentAlt = fake;}

  // take currentAlt, clamp and scale it to strip size, multiply by 2 so we can "overflow" and indicate when over MAX_ALTITUDE
  uint8_t scaledWings = constrain(map(currentAlt, 0, MAX_ALTIMETER, 0, Right.stopPoint), 0, Right.StopPoint) * 2;
  uint8_t scaledFuse = constrain(map(currentAlt, 0, MAX_ALTIMETER, 0, Fuse.stopPoint), 0, Fuse.StopPoint) * 2;

  for (uint8_t i = 0; i < Right.stopPoint; i++) {
    CRGB color = CRGB::Black;
    if (i < scaledWings) {
      if (i < (scaledWings - Right.stopPoint)) {
        color = CRGB::Orange;
      } else {
        color = CRGB::White;
      }
    }
    Right.set(i, color);
    Left.set(i, color);
  }

  for (uint8_t i = 0; i < Fuse.stopPoint; i++) {
    CRGB color = CRGB::Black;
    if (i < scaledFuse) {
      if (i < (scaledFuse - Fuse.stopPoint)) {
        color = CRGB::Orange;
      } else {
        color = CRGB::White;
      }
    }
    Fuse.set(i, color);
  }

  // map vertical speed value to gradient palette
  int vspeedMap;
  avgVSpeed[0]=avgVSpeed[1];
  avgVSpeed[1]=avgVSpeed[2];
  avgVSpeed[2]=(currentAlt-prevAlt)*100; // *100 just gets things into int territory so we can work with it easier
  vSpeed = (avgVSpeed[0]+avgVSpeed[1]+avgVSpeed[2])/3;
  vSpeed = constrain(vSpeed, -interval, interval);

  vspeedMap = map(vSpeed, -interval, interval, 0, 240);

  fill_solid(Tail.leds, TAIL_LEDS, ColorFromPalette(palette, vspeedMap));

  Serial.print(F("Current relative altitude:  "));
  Serial.print(currentAlt);
  Serial.print(F("\t\tVSpeed: "));
  Serial.print(vSpeed);
  Serial.print(F("\tVSpeedMap: "));
  Serial.println(vspeedMap);
  
  prevAlt = currentAlt;

  showStrip();
}

// Enum: twinkle
// ----------------------------
//   some named "states" for the twinkle functions
enum {SteadyDim, Dimming, Brightening};

// Function: doTwinkle1
// --------------------
//   helper function for the twinkle show
//
//   ledArray: pointer to the led array we're modifying
//   pixelState: pointer to the array that holds state info for the led array
//   size: size of the led array
void doTwinkle1(CRGB * ledArray, uint8_t * pixelState, uint8_t size) {
  const CRGB colorDown = CRGB(1, 1, 1);
  const CRGB colorUp = CRGB(8, 8, 8);
  const CRGB colorMax = CRGB(128, 128, 128);
  const CRGB colorMin = CRGB(0, 0, 0);
  const uint8_t twinkleChance = 1;

  for (int i = 0; i < size; i++) {
    if (pixelState[i] == SteadyDim) {
      // if the pixel is steady dim, it has a random change to start brightening
      if (random8() < twinkleChance) {
        pixelState[i] = Brightening;
      }
      if (prevShow != currentShow) { // Reset all LEDs at start of show
        ledArray[i] = colorMin;
      }
    }

    if (pixelState[i] == Brightening) {
      // if it's brightening, once max, start dimming. otherwise keep going up
      if (ledArray[i] >= colorMax) {
        pixelState[i] = Dimming;
      } else {
        ledArray[i] += colorUp;
      }
    }

    if (pixelState[i] == Dimming) {
      // if dimming, once all the way dim, stop. otherwise, keep dimming
      if (ledArray[i] <= colorMin) {
        ledArray[i] = colorMin;
        pixelState[i] = SteadyDim;
      } else {
        ledArray[i] -= colorDown;
      }
    }
  }
}

// Function: twinkle1
// ----------------------------
//   random twinkle effect on all LEDs
void twinkle1 () {
  // arrays to hold the "state" of each LED of each strip
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

  doTwinkle1(Right.leds, pixelStateRight, Right.stopPoint);
  doTwinkle1(Left.leds,  pixelStateLeft, Left.stopPoint);
  doTwinkle1(Nose.leds,  pixelStateNose, NOSE_LEDS);
  doTwinkle1(Fuse.leds,  pixelStateFuse, FUSE_LEDS);
  doTwinkle1(Tail.leds,  pixelStateTail, TAIL_LEDS);

  interval = 10;
  showStrip();
}

// Function: statusFlash overloads
// -------------------------------
//   various overload versions of the statusFlash function
void statusFlash(bool status) {
  if (status) { statusFlash('g', 4, 50); }
  else { statusFlash('r', 4, 50); }
}

void statusFlash(bool status, uint8_t numFlashes, int delay_time) {
  if (status) { statusFlash('g', numFlashes, delay_time); }
  else { statusFlash('r', numFlashes, delay_time); }
}

void statusFlash(char status) {
  statusFlash(status, 4, 50);
}

// Function: statusFlash
// ---------------------
//   flashes red/green/white for different program mode indicators
//
//   status: single char ('w', 'g', or 'r') that specifies what color to flassh all leds
//   numFlashes: how many on/off cycles to do
//   delay_time: delay after both the 'on' and 'off' states
void statusFlash(char status, uint8_t numFlashes, int delay_time) {
  CRGB color;
  switch (status) {
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
  for (int j = 0; j < numFlashes; j++) {
    fill_solid(Right.leds, Right.stopPoint, color);
    fill_solid(Left.leds, Left.stopPoint, color);
    fill_solid(Nose.leds, NOSE_LEDS, color);
    fill_solid(Fuse.leds, FUSE_LEDS, color);
    fill_solid(Tail.leds, TAIL_LEDS, color);
    digitalWrite(LED_BUILTIN, HIGH);
    showStrip();
    delay(delay_time);

    fill_solid(Right.leds, Right.stopPoint, CRGB::Black);
    fill_solid(Left.leds, Left.stopPoint, CRGB::Black);
    fill_solid(Nose.leds, NOSE_LEDS, CRGB::Black);
    fill_solid(Fuse.leds, FUSE_LEDS, CRGB::Black);
    fill_solid(Tail.leds, TAIL_LEDS, CRGB::Black);
    digitalWrite(LED_BUILTIN, LOW);
    showStrip();
    delay(delay_time);
  }
  digitalWrite(LED_BUILTIN, LOW);
}
