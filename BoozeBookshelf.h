// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef BoozeBookshelf2_H_
#define BoozeBookshelf2_H_
#include "Arduino.h"
#include "EEPROM.h"
#include "LEDFader.h"
//add your includes for the project BoozeBookshelf here


//end of add your includes here
#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

//add your function definitions for the project BoozeBookshelf here


// Grid numbers
#define SHELVES 4 // Number of shelves
#define RGB 3     // Number of LED colors per shelf (R, G, B)

// Proximity sensor range distance values in millimeters
#define CLOSE_RANGE 850
#define MED_RANGE 1000
#define OUT_OF_RANGE_DELAY 500 // Number of seconds to wait before fading out when a person goes out of range

// Color fade
#define FADE_SPEED 2000

// Color cross fade directions
#define COLOR_FADE_RIGHT 1
#define COLOR_FADE_LEFT -1

// IR Codes
#define IR_POWER 'P'
#define IR_A 'A'
#define IR_B 'B'
#define IR_C 'C'
#define IR_UP 'u'
#define IR_DOWN 'd'
#define IR_LEFT 'l'
#define IR_RIGHT 'r'
#define IR_SELECT 's'


int wrap(int val, int min, int max);
void process_range();
bool change_program();
int get_distance();
void off();
void set_led(int shelf, int led, int value);
void set_shelf(int shelf, int r, int g, int b);
void set_all(int r, int g, int b);
void fade_shelf(byte shelf, int r, int g, int b, int duration);
void fade_all(int pwm);
void fade_all(int r, int g, int b);
void fade_all(int pwm, int duration);
void fade_all(int r, int g, int b, int duration);
void change_speed(int by);
bool do_fade_step();
bool update_shelf(byte shelf);
void cancel_fade();
void init_program_1();
void run_program_1();
void prog1_set_colors();
void init_program_2();
void run_program_2();
void prog2_color_cross_fade(int direction, int increment);
void init_program_3();
void run_program_3();
void blink_program_3();

class Program {
  public:
    void run();
};

class Program1 : public Program {
  // The fade direction for each shelf (1 = up, -1 = down)
  int direction[SHELVES];

  // The duration for each shelf fade
  int duration[SHELVES];

  public:
    Program1();
    void run();
};

/*
 -------------------------
 Program 2
 Cross fade RGB values between colors from Red to Green to Blue
 Use the remote Up/Down arrows to make the transition faster or slower.
 -------------------------
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

/*
 -------------------------
 Program 3
 Manually select a color with the remote

 Right/Left: Chooses the color to mix (Red, Green, Blue). The color will briefly flash.
 Up/Down:    Changes the intensity of that color
 Select:     Clears the colors.
 -------------------------
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
