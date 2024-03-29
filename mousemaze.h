#ifndef MOUSEMAZE_H
#define MOUSEMAZE_H

#define DEBUG    0      // Set this to 1 for debug serial printing
#define joyhor   0      // Analog pin 0
#define joyver   1      // Analog pin 1
#define joypush  11     // Digital pin 11
#define joyerr   50     // Joystick discrepancy

#define nopthled 13     // No Path LED: Digital pin 13
#define buttonpause 12  // Pause button: Digital pin 12

// Standard U of A Library Settings, Assume Atmel Mega SPI pins
#define SD_CS    5  // Chip select line for SD card
#define TFT_CS   6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)

// the board is 9 x 9 points large
#define map_x    9
#define map_y    9
#define null     255
#define wall_max 144
#define mouse_start 0
#define cheese_start 70
// offset point drawings by 3 pixels
#define offset   3
#define visible_pt 71


uint8_t getSeed();
void initialize();
void initialize_joy();
void initialize_cheese();
void initialize_map(point * point_array);
void initialize_null_walls();
void initialize_rand_walls(point * point_array);
uint8_t * get_options(uint8_t pointvalue, uint8_t * options);
void random_cheese();
void draw_corners(point *point_array);
void draw_walls();
void drawtext(char *text);

uint8_t user_walls();
uint8_t yes_or_no();
uint8_t read_joy_input();

void setup();
void loop();

#endif
