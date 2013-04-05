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

// Necessary global variables go here
uint16_t joycenx;   // center value for x, should be around 512
uint16_t joyceny;   // center value for y, should be around 512
// the board is 9 x 9 points large
#define map_x 9
#define map_y 9
point * point_array;
point mouse;
point cheese;
wall * wall_array;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Forward declarations of project functions go here
void initialize();
void initialize_joy();
void setup();
void loop();

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

  for (int y = 0; y < map_y; y++) {
      int y_coord =  y * 15;
    for (int x = 0; x < map_x; x++) {
      int x_coord = x * 15;
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
  return point_array;
}

void initialize_cheese() {
  /*
    place the cheese randomly on board, but in middle of box
  */
  int x_coord = rand() % 128;
  int y_coord = rand() % 128;
  // need to be a multiple of 7
  x_coord = x_coord / 7;
  y_coord = y_coord / 7;
}

void initialize_rand_walls() {
  /*
    place walls randomly in map
   */

  // FIX MATH HERE!!

  // malloc enough for every wall in the map
  wall_array = (typeof(wall_array)) malloc(sizeof(*wall_array) * 100);

  uint8_t pt1, pt2;
  // pick random no. of walls to draw, within reason
  uint8_t no_walls = rand() % 50;

  if (DEBUG == 1) {
    Serial.print("Printing walls: ");
    Serial.println(no_walls);
  }

  // choose random pt1
  // pt2 must be pt1 +- 1 or +- 10
  for (int i = 0; i < no_walls; i++) {
    pt1 = rand() % 81;
    uint8_t options [4] = {pt1 - 1, pt1 + 1, pt1 - 10, pt1 + 10};
    pt2 = options[rand() % sizeof(options)/sizeof(options[0])];

    wall_array[i].pt1 = point_array[pt1];
    wall_array[i].pt2 = point_array[pt2];

    if (DEBUG == 1) {
      Serial.print(i);
      Serial.print(": ");
      Serial.print(wall_array[i].pt1.num);
      Serial.print(", ");
      Serial.println(wall_array[i].pt2.num);
      }
  }

}

void setup() {
  initialize();
  initialize_joy();
  point_array = initialize_map();
  initialize_cheese();

  // put mouse in middle of top right corner box
  mouse.x_coord = 7;
  mouse.y_coord = 7;

  initialize_rand_walls();
}

void loop() {
  
}
