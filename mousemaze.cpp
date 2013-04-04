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
  point *point_array;
  point_array = (typeof(point_array)) malloc(sizeof(*point_array) * num_points);
  // always check the return code of malloc
  if ( point_array == 0 ) {
    Serial.println("No memory!");
    while ( 1 ) {}
  }
  
  point_array[1].x_coord = 0;
  point_array[1].y_coord = 0;
  
  for (int y = 0; y <= map_y; y++) {
    for (int x = 0; x <= map_x; x++) {
      point_array[x + y + 1].x_coord = point_array[x + y].x_coord + 15;
      point_array[x + y + 1].y_coord = point_array[x + y].y_coord + 15;
    }
  }
  return point_array;
}

void setup() {
  initialize();
  initialize_joy();
  point* point_array = initialize_map();
  for (int i = 0; i < map_x * map_y; i++) {
    Serial.println(point_array[i].x_coord);
  }
}

void loop() {
  
}
