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


// The pins that the LED strips are plugged into as a
// multi-dimensional array of [shelves][color]
LEDFader shelves[SHELVES][RGB] = {
    // Red         Green        Blue
    LEDFader(4), LEDFader(3), LEDFader(2),   // Shelf 1 (top)
    LEDFader(5), LEDFader(7), LEDFader(6),   // Shelf 2
    LEDFader(8), LEDFader(9), LEDFader(10),  // Shelf 3
    LEDFader(11), LEDFader(12), LEDFader(13) // Shelf 4 (bottom)
};

// True if any shelves are currently fading
bool is_fading = false;

// Proximity sensor range state
int distance = 0;          // current distance

// The last IR code received
char ir_value = 0;

// The LED program running
Program* current_program;
byte current_program_num = 0;

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

  // Default program
  current_program = new Program0();
}

void loop() {

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
  run_program();
}

/**
 * Utility function, like 'constrain', but when val is larger than max, it becomes min
 * and when it is less than min, it becomes max
 */
int wrap(int val, int min, int max){
  if (val < min)
    return max;
  if (val > max)
    return min;
  return val;
}

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
byte run_program() {
  byte last_prog = current_program_num;

  // IR Command recieved
  if (Serial1.available()) {
    ir_value = Serial1.read();
    Serial.println(ir_value);

    // Start new program
    switch(ir_value) {
      case IR_POWER: // Turn off custom program and go back to the default behavior
        current_program_num = 0;
        current_program = new Program0();
      break;
      case IR_A:
        current_program_num = 1;
        current_program = new Program1();
      break;
      case IR_B:
        current_program_num = 2;
        current_program = new Program2();
      break;
      case IR_C:
        current_program_num = 3;
        current_program = new Program3();
      break;
    }

    // Program changed
    if (last_prog != current_program_num) {
      Serial.print("Start program ");
      Serial.println(current_program_num);
    }

  } else {
    ir_value = 0;
  }

  // Run program
  current_program->run();

  return current_program_num;
}


/**
 * Get the distance, in mm, from the MaxSonar sensor
 * (code adapted from http://forum.arduino.cc/index.php?topic=130842.0)
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

/**
 * Turn off all LEDs
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

/**
 * Set the PWM value on a single LED of a shelf
 */
void set_led(byte shelf, byte led, byte value) {
  shelves[shelf][led].set_value(value);
}

/**
 * Set the RGB value of a shelf.
 */
void set_shelf(byte shelf, byte r, byte g, byte b) {
  shelves[shelf][0].set_value(r);
  shelves[shelf][1].set_value(g);
  shelves[shelf][2].set_value(b);
}

/**
 * Set all shelves to the same RGB color
 */
void set_all(byte r, byte g, byte b) {
  for (byte shelf = 0; shelf < SHELVES; shelf++) {
    set_shelf(shelf, r, g, b);
  }
}

/**
 * Fade a shelf to an RGB color
 */
void fade_shelf(byte shelf, byte r, byte g, byte b, int duration) {
  shelves[shelf][0].fade((uint8_t)r, duration);
  shelves[shelf][1].fade((uint8_t)g, duration);
  shelves[shelf][2].fade((uint8_t)b, duration);
}

/**
 * Fade all shelves to white, at an intensity between 0 - 255
 */
void fade_all(byte pwm, int duration) {
  fade_all(pwm, pwm, pwm, duration);
}

/**
 * Fade all shelves to the same RGB value.
 */
void fade_all(byte r, byte g, byte b, int duration) {
  for (byte shelf = 0; shelf < SHELVES; shelf++) {
    fade_shelf(shelf, r, g, b, duration);
  }
}

/**
 * Adjust the current fade speed by this many milliseconds, up or down
 * For example, to slow it down by 100 milliseconds, pass -100
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

/**
 * Returns true if the shelf is still fading
 */
bool is_shelf_fading(byte shelf) {
  return (shelves[shelf][0].is_fading()
      || shelves[shelf][1].is_fading()
      || shelves[shelf][2].is_fading());
}

/*
 -------------------------
 Program 0
 The default program that fades the LEDs Up when someone walks up to the bookshelf and
 fades them down when the person walks away.
 -------------------------
*/
Program0::Program0() {
  Serial.println("Init program 0");

  range_close = false;
  range_medium = false;
  out_range_timer = 0;

  // Fade out all LEDs
  fade_all(0, 0, 0, 1000);
}

/**
 * Get the distance from the proximity sensor and adjust the LEDs according to how close the person is.
 * Within 75cm, the LEDs turn on to 50%
 * Within 50cm, the LEDs turn on to 100%
 * Otherwise, turn the LEDs off.
 */
void Program0::run() {
  int distance = get_distance();

  // Invalid distance, too close to for the sensor
  if (distance <= 30) {
    return;
  }

  // Out of range
  if (distance > MED_RANGE ){

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
      fade_all(0, FADE_SPEED);
    }
  }

  // Medium range
  if (distance <= MED_RANGE && distance > CLOSE_RANGE && !range_medium && !range_close) {
    Serial.println("Medium range");
    range_close = false;
    range_medium = true;
    out_range_timer = 0;
    fade_all(30, FADE_SPEED);
  }

  // Close range
  if (!range_close && distance <= CLOSE_RANGE) {
    Serial.println("Close range");
    range_close = true;
    range_medium = true;
    out_range_timer = 0;
    fade_all(255, FADE_SPEED);
  }
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
    if(is_shelf_fading(s) == false) {

      // Fade down
      if (direction[s] == 1) {
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


        Serial.print("Fade up shelf ");
        Serial.println(s);

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
  fade_all(colors[0], colors[1], colors[2], 500);
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
