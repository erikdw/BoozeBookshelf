// Do not remove the include below
#include "BoozeBookshelf.h"

/**

LED controller for a booze bookshelf
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The bookshelf has a couple RGB LED strips installed
at each level and uses a Sharp IR proximity sensor to detect
when someone walks up to the bookshelf.

When someone comes within a medium range of 75 cm (30 in), the
shelves light up to about 50%.

If the person moves closer than 50 cm, the shelves light up to 100%

Fun enhancement
---------------
Using a simple IR receiver and the SparkFun IR remote control (https://www.sparkfun.com/products/11759)
the user can start several lighting programs or turn the LEDs to a specific hue and brightness.

*/


// The pins that the LED strips are plugged into.
// multi-dimensional array of [shelves][color] where:
//  * leds[0] is the top shelf
//  * color is always R, G, B
LEDFader shelves[SHELVES][RGB] = {
    LEDFader(4), LEDFader(3), LEDFader(2),
    LEDFader(5), LEDFader(7), LEDFader(6),
    LEDFader(8), LEDFader(9), LEDFader(10),
    LEDFader(11), LEDFader(12), LEDFader(13)
};

// True if any shelves are currently fading
bool is_fading = false;

// Proximity sensor range state
int distance = 0;          // current distance
bool range_medium = false; // medium range <= 75cm
bool range_close = false;  // close range <= 50cm
unsigned long out_range_timer = 0; // The time to dim the LEDs when a person goes out of range

char ir_value = 0;

void* custom_program;
byte custom_program_num = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Start");

  delay(500);
  off();
  delay(500);

  // IR receiver
  Serial1.begin(115200);

  // Distance sensor
  Serial2.begin(9600);
}

int count = 0;
void loop() {

  if (count > 100) {
    //return;
  }
  count++;

  // Update all LEDs
  is_fading = false;
  for (byte shelf = 0; shelf < SHELVES; shelf++) {
    for(byte rgb = 0; rgb < RGB; rgb++) {
      if (shelves[shelf][rgb].update()) {
        is_fading = true;
      }
    }
  }

  // Run program
  bool skip_default = change_program();
  if (!skip_default) {
    process_range();
  }
}

/*
  Utility function, like 'constrain', but when val is larger than max, it becomes min
  and when it is less than min, it becomes max
*/
int wrap(int val, int min, int max){
  if (val < min)
    return max;
  if (val > max)
    return min;
  return val;
}

/*
  Get the distance from the proximity sensor and adjust the LEDs according to how close the person is.
  Within 75cm, the LEDs turn on to 50%
  Within 50cm, the LEDs turn on to 100%
  Otherwise, turn the LEDs off.
*/
void process_range() {
  get_distance();

  // Invalid distance
  if (distance <= 30) {
    return;
  }

  // Out of range
  if (distance > MED_RANGE){

    // If was formerly in range, set timer and dim
    if (range_medium || range_close) {
      range_close = false;
      range_medium = false;
      Serial.println("Went out of range...");

      out_range_timer = millis() + OUT_OF_RANGE_DELAY;

      // Handle when value wraps back to zero (just add 1 for simplicity)
      if (out_range_timer == 0) {
        out_range_timer = 1;
      }
    }

    // Timer set and fired, fade out
    else if(out_range_timer > 0 && out_range_timer <= millis()) {
      Serial.println("Dim lights");
      out_range_timer = 0;
      fade_all(0);
    }
  }

  // Medium range
  if (distance <= MED_RANGE && distance > CLOSE_RANGE && !range_medium && !range_close) {
    Serial.println("Medium range");
    range_close = false;
    range_medium = true;
    out_range_timer = 0;
    fade_all(30);
  }

  // Close range
  if (!range_close && distance <= CLOSE_RANGE) {
    Serial.println("Close range");
    range_close = true;
    range_medium = true;
    out_range_timer = 0;
    fade_all(255);
  }
}

/*
  Receive codes from the IR receiver and move to a program:

  A - program 1
  B - program 2
  C - program 3
  POWER - Switch back to the default behavior (turn on when someone approaches the bookshelf)

  Returns true if a program is running, or false to run the default behavior.
*/
bool change_program() {
  // IR Command recieved
  int last_prog = custom_program_num;

  if (Serial1.available()) {
    ir_value = Serial1.read();
    Serial.println(ir_value);
    switch(ir_value) {
      case IR_POWER: // Turn off custom program and go back to the default behavior
        custom_program_num = 0;
        off();
        range_close = false;
        range_medium = false;
        return false;
      case IR_A:
        custom_program_num = 1;
        custom_program = new Program1();
        //init_program_1();
      break;
      case IR_B:
        custom_program_num = 2;
        custom_program = new Program2();
        //init_program_2();
      break;
      case IR_C:
        custom_program_num = 3;
        custom_program = new Program3();
        //init_program_3();
      break;
    }

    // Program changed
    if (last_prog != custom_program_num) {
      Serial.print("Start program ");
      Serial.println(custom_program_num);
    }

  } else {
    ir_value = 0;
  }

  // Run custom programs
  switch (custom_program_num) {
    case 1:
      ((Program1 *)custom_program)->run();
//      run_program_1();
    break;
    case 2:
      ((Program2 *)custom_program)->run();
//      run_program_2();
    break;
    case 3:
      ((Program3 *)custom_program)->run();
//      run_program_3();
    break;
    default:
      return false;
  }

  return (custom_program_num > 0);
}

/*
  Get the distance, in mm, from the MaxSonar sensor
  (code adapted from http://forum.arduino.cc/index.php?topic=130842.0)
*/
int get_distance() {
  if((char)Serial2.peek() == 'R'){
    if(Serial2.available() >= 5) {
      Serial2.read(); // Take R off the top
      int thousands = (Serial2.read() - '0') * 1000; // Take and convert each range digit to human-readable integer format.
      int hundreds = (Serial2.read() - '0') * 100;
      int tens = (Serial2.read() - '0') * 10;
      int units = (Serial2.read() - '0') * 1;
      int cr = Serial2.read(); // Don't do anything with this, just clear it out of the buffer with the rest.

      // Assemble the digits into the range integer.
      distance = thousands + hundreds + tens + units;

      // Something is within range HRLV-EZ1 range is 30cm - 5m
      //Serial.print("Range (mm): ");
      //Serial.println(String(distance));
      return distance;
    }
  }
  // Take the peeked byte off the top
  else {
    Serial2.read();
  }

  // Return the last distance received
  return distance;
}

/*
 Turn off all LEDs
*/
void off() {
  for (int shelf = 0; shelf < SHELVES; shelf++) {
    for (int rgb = 0; rgb < RGB; rgb++) {
      LEDFader *led = &shelves[shelf][rgb];
      led->stop_fade();
      led->set_value(0);
    }
  }
}

/*
  Set a PWM value on a single LED.
  Use this instead of analogWrite, so the color value will be saved in the color array matrix.
*/
void set_led(int shelf, int led, int value) {
  shelves[shelf][led].set_value(value);
}

/*
 Set all shelves to the same color
*/
void set_shelf(int shelf, int r, int g, int b) {
  shelves[shelf][0].set_value(r);
  shelves[shelf][1].set_value(g);
  shelves[shelf][2].set_value(b);
}

/*
  Set all shelves to the same color
*/
void set_all(int r, int g, int b) {
  for (byte shelf = 0; shelf < SHELVES; shelf++) {
    set_shelf(shelf, r, g, b);
  }
}

/*
  Fade a shelf to a color
*/
void fade_shelf(byte shelf, int r, int g, int b, int duration) {
  r = constrain(r, 0, 256);
  g = constrain(g, 0, 256);
  b = constrain(b, 0, 256);
  shelves[shelf][0].fade((uint8_t)r, duration);
  shelves[shelf][1].fade((uint8_t)g, duration);
  shelves[shelf][2].fade((uint8_t)b, duration);
}

/*
  Fade all shelves to an RGB value
*/
void fade_all(int pwm) {
  fade_all(pwm, pwm, pwm);
}
void fade_all(int r, int g, int b) {
  fade_all(r, g, b, FADE_SPEED);
}
void fade_all(int pwm, int duration) {
  fade_all(pwm, pwm, pwm, duration);
}
void fade_all(int r, int g, int b, int duration) {
  for (byte shelf = 0; shelf < SHELVES; shelf++) {
    fade_shelf(shelf, r, g, b, duration);
  }
}

/*
 * Ajust the current fade speed by this many milliseconds
 */
void change_speed(int by) {
  for (int shelf = 0; shelf < SHELVES; shelf++) {
    for (int rgb = 0; rgb < RGB; rgb++) {

      if (by < 0) {
        shelves[shelf][rgb].faster(abs(by));
      }
      else {
        shelves[shelf][rgb].slower(by);
      }
    }
  }
}

/*
  Fade all the LEDs
*/
bool do_fade_step() {
  bool ret = false;
  for (int shelf = 0; shelf < SHELVES; shelf++) {
    if (update_shelf(shelf)) {
      ret = true;
    }
  }
  return ret;
}

/*
 Run an update on all LEDs of a shelf and return true
 if any are still fading
*/
bool update_shelf(byte shelf) {
  return (shelves[shelf][0].update()
      || shelves[shelf][1].update()
      || shelves[shelf][2].update());
}

/*
 -------------------------
 Program 1
 Fade a random color up/down on each shelf independant of all other shelves
 -------------------------
*/
Program1::Program1() {
  Serial.println("Init program 1");
  off();
  randomSeed(analogRead(0));

  for (int i = 0; i < SHELVES; i++) {
    direction[i] = 0;
    duration[i] = 0;
  }
}

// The main part of the program, run once each loop() cycle
void Program1::run() {

  // Loop over the shelves and set colors when their done fading
  for (byte s = 0; s < SHELVES; s++) {

    // Change color/direction if fading is done
    if(update_shelf(s) == false) {

      // Fade down
      if (direction[s] == 1) {
        if (s == 1) {
          Serial.print("Fade down ");
          Serial.print(s);
          Serial.print(" -> ");
          Serial.println(duration[s]);
        }
        fade_shelf(s, 0, 0, 0, duration[s]);
        direction[s] = -1;
      }

      // Fade up
      else {
        if (s == 1) {
          Serial.print("Change color ");
          Serial.println(s);
        }

        // Generate random pwm on two colors
        byte colors[3] = {0,0,0};
        for(byte i = 0; i < 2; i++) {
          byte rgb = random(0, 3);

          // First color (100 - 256)
          if (i == 0) {
            colors[rgb] = random(100, 256);
          }
          // Second color (0 - 256)
          else {
            colors[rgb] = random(0, 256);
          }
        }

        // Random duration
        int shelf_duration = random(1000, 2000);

        // Start fade
        fade_shelf(s, colors[0], colors[1], colors[2], shelf_duration);
        duration[s] = shelf_duration;
        direction[s] = 1;


        if (s == 1) {
          Serial.print(colors[0]);
          Serial.print(", ");
          Serial.print(colors[1]);
          Serial.print(", ");
          Serial.print(colors[2]);
          Serial.print(" -> ");
          Serial.println(duration[s]);
        }
      }
    }
  }
}

/*
 -------------------------
 Custom program 1
 Fade a random color on each shelf
 -------------------------
*/
int prog1_dirs[SHELVES] = {0,0,0,0};
int prod1_durations[SHELVES] = {0,0,0,0};
void init_program_1(){
  Serial.println("Init program 1");
  off();
  randomSeed(analogRead(0));
}
void run_program_1(){

  // Loop over the shelves and set colors when their done fading
  for (byte s = 0; s < SHELVES; s++) {

    // Change color/direction if fading is done
    if(update_shelf(s) == false) {

      // Fade down
      if (prog1_dirs[s] == 1) {
        fade_shelf(s, 0, 0, 0, prod1_durations[s]);
        prog1_dirs[s] = -1;
      }

      // Fade up
      else {
        // Generate random pwm on two colors between 75 and 256
        byte colors[3] = {0,0,0};
        for(byte i = 0; i < 2; i++) {
          byte rgb = random(0, 3);

          // Primary color
          if (i == 0) {
            colors[rgb] = random(100, 256);
          }
          // Secondary
          else {
            colors[rgb] = random(0, 256);
          }
        }

        // Random duration
        int duration = random(1000, 2000);

        // Start fade
        fade_shelf(s, colors[0], colors[1], colors[2], duration);
        prod1_durations[s] = duration;
        prog1_dirs[s] = 1;
      }
    }
  }
}

/*
 -------------------------
 Program 2
 Cross fade RGB values between colors from Red to Green to Blue
 Use the remote Up/Down arrows to make the transition faster or slower.
 -------------------------
*/
Program2::Program2() {
  *colors = (0,0,0);
  speed = 3000;
  index = 0;
}

// The main part of the program, run once each loop() cycle
void Program2::run() {

  // Adjust speed
  if (ir_value == IR_DOWN) {
    speed += 100;
    change_speed(100);

    Serial.print("Slow down: ");
    Serial.println(speed);
  }
  else if (ir_value == IR_UP) {
    speed -= 100;

    // Minimum is 500ms
    if (speed  < 500) {
      speed  = 500;
    }
    change_speed(-100);

    Serial.print("Speed up: ");
    Serial.println(speed);
  }

  // Move to the next color if fading on all shelves is done
  if (!is_fading) {
    colors[index] = 0;            // current color fades to 0
    index = wrap(++index, 0, 2);  // Increment index and wrap to 0, if greater than 2
    colors[index] = 255;          // next color fades to 255

    Serial.print("Move to the next color");
    Serial.println(index);

    Serial.print(colors[0]);
    Serial.print(", ");
    Serial.print(colors[1]);
    Serial.print(", ");
    Serial.println(colors[2]);

    // Fade to the next color
    fade_all(colors[0], colors[1], colors[2], speed);
  }
}


/*
 -------------------------
 Custom program 2
 Cross fade RGB values between colors from Red to Green to Blue
 -------------------------
*/
int prog2_colors[] = {0, 0, 0};
int prog2_index = 0;
int prog2_speed = 3000;

void init_program_2(){
  //fade_all(0, 0, 0);
}
void run_program_2(){
  // Adjust speed
  if (ir_value == IR_DOWN) {
    prog2_speed += 100;
    change_speed(100);
    Serial.print("Slow down: ");
    Serial.println(prog2_speed);
  }
  else if (ir_value == IR_UP) {
    prog2_speed -= 100;

    if (prog2_speed < 500) {
      prog2_speed = 500;
    }

    change_speed(-100);
    Serial.print("Speed up: ");
    Serial.println(prog2_speed);
  }

  // Move to the next color
  if (!is_fading) {
    prog2_colors[prog2_index] = 0;
    prog2_index = wrap(++prog2_index, 0, 2);
    prog2_colors[prog2_index] = 255;

    Serial.print("Move to the next color");
    Serial.println(prog2_index);

    Serial.print(prog2_colors[0]);
    Serial.print(", ");
    Serial.print(prog2_colors[1]);
    Serial.print(", ");
    Serial.println(prog2_colors[2]);

    fade_all(prog2_colors[0], prog2_colors[1], prog2_colors[2], prog2_speed);
  }
}

/*
 -------------------------
 Program 3
 Manually select a color with the remote

 Right/Left: Chooses the color to mix (Red, Green, Blue). The color will briefly flash.
 Up/Down:    Changes the intensity of that color
 Select:     Clears the colors.
 -------------------------
*/
Program3::Program3() {
  inc = 20;
  color_select = 0;
  *colors = (0,0,0);
  last_ir = 0;

  // Load previously used colors from EEPROM: select, r, g, b
  byte select = EEPROM.read(0);
  if (select <= 2) {
    colors[0] = constrain(EEPROM.read(1), 0, 255);
    colors[1] = constrain(EEPROM.read(2), 0, 255);
    colors[2] = constrain(EEPROM.read(3), 0, 255);

    // Fade to the color
    fade_all(colors[0], colors[1], colors[2], 500);
    delay(500);
  }
  else {
    off();
  }

  blink();
}

// Blink the color that is selected
void Program3::blink(){
  int blink_colors[3] = {0,0,0};
  blink_colors[color_select] = 150;

  set_all(blink_colors[0], blink_colors[1], blink_colors[2]);
  delay(400);
  fade_all(colors[0], colors[1], colors[2]);
}

// Save the current values to the EEPROM
void Program3::save(){
  EEPROM.write(0, color_select);
  EEPROM.write(1, colors[0]);
  EEPROM.write(2, colors[1]);
  EEPROM.write(3, colors[2]);
}

// The main part of the program, run once each loop() cycle
void Program3::run() {
  if (ir_value) {

    int color = colors[color_select];

    switch(ir_value) {
    // Reset the colors
    case IR_SELECT:
      colors[0] = 0;
      colors[1] = 0;
      colors[2] = 0;
    break;

    // Increase selected color
    case IR_UP:
      color += inc;
      Serial.print("Increase color ");
      Serial.println(color_select);
    break;

    // Decrease selected color
    case IR_DOWN:
      color -= inc;
      Serial.print("Decrease color ");
      Serial.println(color_select);
    break;

    // Move to the next color
    case IR_RIGHT:
      color_select++;
      color_select = wrap(color_select, 0, 2);
      color = colors[color_select];

      Serial.print("Move color to");
      Serial.println(color_select);

      blink();
    break;

    // Move to previous color
    case IR_LEFT:
      color_select--;
      color_select = wrap(color_select, 0, 2);
      color = colors[color_select];

      Serial.print("Move color to");
      Serial.println(color_select);

      blink();
    break;
    }
    ir_value = 0;

    // Set colors
    color = constrain(color, 0, 255);
    colors[color_select] = color;
    fade_all(colors[0], colors[1], colors[2], 100);

    Serial.print(colors[0]);
    Serial.print(", ");
    Serial.print(colors[1]);
    Serial.print(", ");
    Serial.println(colors[2]);

    // Save values to EEPROM
    save();
  }
}

/*
 -------------------------
 Custom program 4
 Select the color with the remote
 Start with Red, move Up/Down to adjust the intensity of that color.
 Move right to adjust Green.
 Move right to adjust Blue.
 Move right to go back to Red. (move left to go the opposite direction)
 Press select to clear colors.
 -------------------------
*/
byte prog3_color_select = 0;
byte prog3_colors[] = {0,0,0};
byte prog3_inc = 20;
int prog3_speed = 100;
char prog3_last_ir = 0;
void init_program_3(){

  // Load colors from EEPROM: select, r, g, b
  byte select = EEPROM.read(0);
  if (select <= 2) {
    prog3_colors[0] = constrain(EEPROM.read(1), 0, 255);
    prog3_colors[1] = constrain(EEPROM.read(2), 0, 255);
    prog3_colors[2] = constrain(EEPROM.read(3), 0, 255);
    fade_all(prog3_colors[0], prog3_colors[1], prog3_colors[2], 1000);
    delay(1000);
  }
  else {
    off();
  }

  blink_program_3();
}
void run_program_3(){
  if (ir_value) {

    int color = prog3_colors[prog3_color_select];

    switch(ir_value) {
    case IR_SELECT:
      prog3_colors[0] = 0;
      prog3_colors[1] = 0;
      prog3_colors[2] = 0;
    break;
    case IR_UP:
      color += prog3_inc;
      Serial.print("Increase color ");
      Serial.println(prog3_color_select);
    break;
    case IR_DOWN:
      color -= prog3_inc;
      Serial.print("Decrease color ");
      Serial.println(prog3_color_select);
    break;
    case IR_RIGHT:
      prog3_color_select++;
      prog3_color_select = wrap(prog3_color_select, 0, 2);
      Serial.print("Move color to");
      Serial.println(prog3_color_select);
      color = prog3_colors[prog3_color_select];
      blink_program_3();
    break;
    case IR_LEFT:
      prog3_color_select--;
      prog3_color_select = wrap(prog3_color_select, 0, 2);
      Serial.print("Move color to");
      Serial.println(prog3_color_select);
      color = prog3_colors[prog3_color_select];
      blink_program_3();
    break;
    }
    ir_value = 0;

    color = constrain(color, 0, 255);
    prog3_colors[prog3_color_select] = color;

    Serial.print(prog3_colors[0]);
    Serial.print(", ");
    Serial.print(prog3_colors[1]);
    Serial.print(", ");
    Serial.println(prog3_colors[2]);

    // Set colors
    fade_all(prog3_colors[0], prog3_colors[1], prog3_colors[2], prog3_speed);

    // Save to EEPROM
    EEPROM.write(0, prog3_color_select);
    EEPROM.write(1, prog3_colors[0]);
    EEPROM.write(2, prog3_colors[1]);
    EEPROM.write(3, prog3_colors[2]);
  }
}

// Blink the color that is selected
void blink_program_3(){
  int colors[3] = {0,0,0};
  colors[prog3_color_select] = 150;

  set_all(colors[0], colors[1], colors[2]);
  delay(400);
  fade_all(prog3_colors[0], prog3_colors[1], prog3_colors[2]);
}
