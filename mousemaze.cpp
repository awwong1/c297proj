/*
  CMPUT 297 - Term Project
  Alexander Wong, Michelle Naylor
  
  Mouse Maze Simulation
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SD.h>
#include <SPI.h>
#include "mem_syms.h"
#include "lcd_image.h"
#include "map.h"

#define DEBUG    1      // Set this to 1 for debug serial printing
#define joyhor   0      // Analog pin 0
#define joyver   1      // Analog pin 1
#define joypush  11     // Digital pin 11
#define joyerr   40     // Joystick discrepancy

#define nopthled 13     // No Path LED: Digital pin 13
#define mazeclr  12     // Clear maze button: Digital pin 12

// Standard U of A Library Settings, Assume Atmel Mega SPI pins
#define SD_CS    5  // Chip select line for SD card
#define TFT_CS   6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)

// the board is 9 x 9 points large
#define map_x    9
#define map_y    9

// Necessary global variables go here
uint16_t joycenx;   // center value for x, should be around 512
uint16_t joyceny;   // center value for y, should be around 512
uint16_t num_walls = 0;
point * point_array;
point mouse;
point cheese;
wall * wall_array;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Forward declarations of project functions go here
void initialize();
void initialize_joy();
point * initialize_map();
void draw_corners();
void draw_walls();
void initialize_cheese();
void initialize_rand_walls();
void setup();
void loop();

uint8_t getSeed(){
  uint8_t seed = 0;
  for(int i = 0; i < 8; i++) {
    seed = seed << 1;
    seed = seed ^ analogRead(i);
  }
  return seed;
}

void initialize() {
  Serial.begin(9600);

  pinMode(nopthled, OUTPUT);

  tft.initR(INITR_REDTAB);
  Serial.print("Initializing SD card... ");
  if (!SD.begin(SD_CS)) {
    Serial.println("Failed!");
    return;
  }
  Serial.print("OK!");

  tft.fillScreen(tft.Color565(0x00, 0x00, 0x00));
}

void initialize_joy() {
  // Centers the joystick
  pinMode(joypush, INPUT);
  digitalWrite(joypush, HIGH);
  joyceny = analogRead(joyver);
  joycenx = analogRead(joyhor);
}

void initialize_mouse() {
  /*
    Place the mouse in the top left corner of the map
   */
  mouse.x_coord = 10;
  mouse.y_coord = 10;
}

void initialize_cheese() {
  /*
    Place the cheese in the bottom right corner of the map
   */
  cheese.x_coord = 128 - 13;
  cheese.y_coord = 128 - 13;
}

void random_cheese() {
  /*
    Place the cheese in a random location on the map
    If the new location is the same as the mouse, 
    jump to a new location
   */
  randomSeed(getSeed());


}

point * initialize_map() {
  /*
    Initialize all points that will appear on the screen
   */
  int num_points = map_x * map_y;
  int count = 0;
  point_array = (typeof(point_array)) malloc(sizeof(*point_array) * num_points);

  // always check the return code of malloc
  if ( point_array == 0 ) {
    Serial.println("No memory!");
    while ( 1 ) {}
  }
  
  point_array[0].x_coord = 0;
  point_array[0].y_coord = 0;

  // Offset by 3 pixels to prevent left side off screen
  for (int y = 0; y < map_y; y++) {
    int y_coord =  (y * 15) + 3;
    for (int x = 0; x < map_x; x++) {
      int x_coord = (x * 15) + 3;
      point_array[count].x_coord = x_coord;
      point_array[count].y_coord = y_coord;
      point_array[count].num = count;

      if (DEBUG == 1) {
	Serial.print(point_array[count].num);
	Serial.print(": ");
	Serial.print(point_array[count].x_coord);
	Serial.print(", ");
	Serial.println(point_array[count].y_coord);
      }
      count++;
    }
  }

  // This is placed here for visuals. Also, walls draw over corners currently.
  draw_corners();
  return point_array;
}

void draw_corners() {
  for (int pixel = 0; pixel < 81; pixel++) {
    tft.drawPixel(point_array[pixel].x_coord, point_array[pixel].y_coord, ST7735_WHITE);
  }
}

void draw_walls() {
  for (int line = 0; line < num_walls; line++) {
    point apoint = wall_array[line].pt1;
    point bpoint = wall_array[line].pt2;
    tft.drawLine(apoint.x_coord, apoint.y_coord, bpoint.x_coord, bpoint.y_coord, 0x0990);
  }
}

void draw_mouse() {
  tft.fillCircle(mouse.x_coord, mouse.y_coord, 4, 0xCCCC);
}

void draw_cheese() {
  tft.fillCircle(cheese.x_coord, cheese.y_coord, 4, ST7735_YELLOW);
}

void initialize_rand_walls() {
  /*
    place walls randomly in map
   */

  // Maximum number of walls is 8 * 9 * 2 = 144 walls
  // malloc enough for every wall in the map
  wall_array = (typeof(wall_array)) malloc(sizeof(*wall_array) * 144);
  uint8_t pt1, pt2;
  randomSeed(getSeed());
  uint8_t no_walls = random(144);

  if (DEBUG == 1) {
    Serial.print("Printing walls, Amount: ");
    Serial.println(no_walls);
  }

  // For every point 1, point 2 must be the x/y adjacent point
  // Usually can be +-1 or +-9, with the exception of the edges
  for (int i = 0; i < no_walls; i++) {
    uint8_t leftedge = 0;
    uint8_t rightedge = 0;
    uint8_t topedge = 0;
    uint8_t botedge = 0;
    
    randomSeed(getSeed());
    pt1 = random(81);
    
    // Edge case checker, set boolean values
    if (pt1 < 9) {
      topedge = 1;
    }
    if (pt1 > 71) {
      botedge = 1;
    }
    if ((pt1 % 9) == 0) {
      leftedge = 1;
    }
    if ((pt1 % 9) == 8) {
      rightedge = 1;
    }
    
    uint8_t * options;

    options = (typeof(options)) malloc(sizeof(* options) * 4);

    // always check the return code of malloc
    if ( options == 0 ) {
      Serial.println("No memory!");
      while ( 1 ) {}
    }
    
    for (uint8_t wallchoice = 0; wallchoice < 4; wallchoice++) {  
      if (!topedge) {
	options[wallchoice] = pt1 - 9;
	topedge = 1;
      }
      else if (!botedge) {
	options[wallchoice] = pt1 + 9;
	botedge = 1;
      }
      else if (!leftedge) {
	options[wallchoice] = pt1 - 1;
	leftedge = 1;
      }
      else if (!rightedge) {
	options[wallchoice] = pt1 + 1;
	rightedge = 1;
      }
      else {
	options[wallchoice] = 255;
      }
    }
    while(1) {
      uint8_t choice = random(4);
      pt2 = options[choice];
      if (pt2 != 255) {
	break;
      }
    }
    
    wall_array[i].pt1 = point_array[pt1];
    wall_array[i].pt2 = point_array[pt2];

    free(options);

    if (DEBUG == 1) {
      Serial.print(i);
      Serial.print(": ");
      Serial.print(wall_array[i].pt1.num);
      Serial.print(", ");
      Serial.println(wall_array[i].pt2.num);
    }
    num_walls++;
  }
}

void setup() {
  initialize();
  initialize_joy();
  point_array = initialize_map();
    
  initialize_mouse();
  initialize_cheese();

  initialize_rand_walls();

  draw_walls();
  draw_corners();
  draw_mouse();
  draw_cheese();
}

void loop() {
  // Two different states; One for wall placement, One for mouse cycle
  
}
