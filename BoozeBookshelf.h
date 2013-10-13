// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef BoozeBookshelf_H_
#define BoozeBookshelf_H_

#include "Arduino.h"
#include "EEPROM.h"
#include "LEDFader.h"

/*
 =================
 Program constants
 =================
 */

// Grid numbers
#define SHELVES 4 // Number of shelves
#define RGB 3     // Number of LED colors per shelf (R, G, B)

// Proximity sensor range distance values in millimeters
#define CLOSE_RANGE 850
#define MED_RANGE 1000
#define OUT_OF_RANGE_DELAY 500 // Number of seconds to wait before fading out when a person goes out of range

// Time it takes to fade the LEDs on/off when someone approaches the bookshelf (in milliseconds)
#define FADE_SPEED 2000

// IR Codes received via Serial from the Arduino mini
#define IR_POWER 'P'
#define IR_A 'A'
#define IR_B 'B'
#define IR_C 'C'
#define IR_UP 'u'
#define IR_DOWN 'd'
#define IR_LEFT 'l'
#define IR_RIGHT 'r'
#define IR_SELECT 's'

/*
 =================
 Program Functions
 =================
 */
void loop();
void setup();

/**
 * Run the current program, or change programs based on the IR remote code received.
 *
 * IR Program codes:
 *
 *  - POWER   Program 0 (default program)
 *  - A       program 1
 *  - B       program 2
 *  - C       program 3
 *
 * Returns the current program number
 */
byte run_program();

/**
 * Utility function, like 'constrain', but when val is larger than max, it becomes min
 * and when it is less than min, it becomes max
 */
int wrap(int val, int min, int max);

/**
 * Get the distance, in mm, from the MaxSonar sensor
 * (code adapted from http://forum.arduino.cc/index.php?topic=130842.0)
 */
int get_distance();

/**
 * Turn off all LEDs
 */
void off();

/**
 * Set the PWM value on a single LED of a shelf
 */
void set_led(byte shelf, byte led, byte value);

/**
 * Set the RGB value of a shelf.
 */
void set_shelf(byte shelf, byte r, byte g, byte b);

/**
 * Set all shelves to the same RGB color
 */
void set_all(byte r, byte g, byte b);

/**
 * Fade a shelf to an RGB color
 */
void fade_shelf(byte shelf, byte r, byte g, byte b, int duration);

/**
 * Fade all shelves to white, at an intensity between 0 - 255
 */
void fade_all(byte pwm, int duration);

/**
 * Fade all shelves to the same RGB value.
 */
void fade_all(byte r, byte g, byte b, int duration);

/**
 * Adjust the current fade speed by this many milliseconds, up or down
 * For example, to slow it down by 100 milliseconds, pass -100
 */
void change_speed(int by);

/**
 * Returns true if the shelf is still fading
 */
bool is_shelf_fading(byte shelf);

/*
 =================
 Program Classes
 =================
 */


/**
 * Abstract program interface that all the programs use
 */
class Program {
  public:
    /**
     * Called once for each loop() cycle, while the program is selected
     */
    virtual void run();
};

/*
 * Program 0
 * The default program that fades the LEDs Up when someone walks up to the bookshelf and
 * fades them down when the person walks away.
 */
class Program0 : public Program {

  // If the person is in close range. (LEDs at 100%) See CLOSE_RANGE constant.
  bool range_close;

  // If the person is at medium range. (LEDS at 50%) See MED_RANGE constant.
  bool range_medium;

  // When to dim the LEDs when a person goes out of range. time() + OUT_OF_RANGE_DELAY
  unsigned long out_range_timer;

  public:
    Program0();
    void run();
};

/**
 * Program 1
 * Fade a random color up/down on each shelf independant of all other shelves
 */
class Program1 : public Program {
  // The fade direction for each shelf (1 = up, -1 = down)
  int direction[SHELVES];

  // The duration for each shelf fade
  int duration[SHELVES];

  public:
    Program1();
    void run();
};

/**
 * Program 2
 * Cross fade RGB values between colors from Red to Green to Blue
 * Use the remote Up/Down arrows to make the transition faster or slower.
 */
class Program2 : public Program{

  // The value of each RGB color
  int colors[RGB];

  // The current color index being increased (R=0, G=1, B=2)
  int index;

  // The duration of the transition from one color to another
  int speed;

  public:
    Program2();
    void run();
};

/**
 * Program 3
 * Manually select a color with the remote
 *
 * Right/Left: Chooses the color to mix (Red, Green, Blue). The color will briefly flash.
 * Up/Down:    Changes the intensity of that color
 * Select:     Clears the colors.
 */
class Program3 : public Program{
private:

  // The color we're currently modifying (0 - 2: R,G,B)
  byte color_select;

  // The value of each color (R,G,B)
  byte colors[RGB];

  // How much to change the color by each time the user presses up/down
  byte inc;

  // The last IR value received
  char last_ir;

  void blink();
  void save();
public:
  Program3();
  void run();
};


//Do not add code below this line
#endif /* BoozeBookshelf_H_ */
