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
uint8_t getSeed();
void initialize();
void initialize_joy();
void initialize_cheese();
point * initialize_map();
void initialize_rand_walls();
uint8_t * get_options(uint8_t pointvalue, uint8_t * options);
void random_cheese();
void draw_corners();
void draw_walls();
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

point * initialize_map() {
  /*
    Initialize all points that will appear on the screen
   */
  
  int num_points = map_x * map_y;
  int count = 0;
  point_array = (typeof(point_array)) malloc(sizeof(*point_array) * num_points);
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
      count++;
    }
  }
  
  // This is placed here for visuals. Also, walls draw over corners currently.
  draw_corners();
  return point_array;
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
  for (uint8_t i = 0; i < no_walls; i++){
    uint8_t * options;
    options = (typeof(options)) malloc(sizeof(* options) * 4);
    // always check the return code of malloc
    if ( options == 0 ) {
      Serial.println("No memory!");
      while ( 1 ) {}
    }
    
    pt1 = random(81);
    options = get_options(pt1, options);
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

uint8_t * get_options(uint8_t pointvalue, uint8_t * options){
  uint8_t leftedge = 0;
  uint8_t rightedge = 0;
  uint8_t topedge = 0;
  uint8_t botedge = 0;

  if (pointvalue < 9) { topedge = 1; }
  if (pointvalue > 71) { botedge = 1; }
  if ((pointvalue % 9) == 0) { leftedge = 1; }
  if ((pointvalue % 9) == 8) { rightedge = 1;}
    
  for (uint8_t wallchoice = 0; wallchoice < 4; wallchoice++) {  
    if (!topedge) {
      options[wallchoice] = pointvalue - 9;
      topedge = 1;
    }
    else if (!botedge) {
      options[wallchoice] = pointvalue + 9;
      botedge = 1;
    }
    else if (!leftedge) {
      options[wallchoice] = pointvalue - 1;
      leftedge = 1;
    }
    else if (!rightedge) {
      options[wallchoice] = pointvalue + 1;
      rightedge = 1;
    }
    // dummy variable: 255 indicates non-existence
    else {
      options[wallchoice] = 255;
    }
  }
  return options;
}

void random_cheese() {
  /*
    Place the cheese in a random location on the map
    If the new location is the same as the mouse, 
    jump to a new location
   */
  while(1) {
    uint8_t location = random(81);
    if (location > 71 || (location % 9 == 8)) {
      continue;
    }
    else {
      cheese.x_coord = point_array[location].x_coord + 7;
      cheese.y_coord = point_array[location].y_coord + 7;
      break;
    }
  }  
}

uint8_t[] bfs(entity mouse, entity cheese, uint8_t[] visited) {
  /*
    performs a breadth first search from mouse to cheese
    returns array with path
   */
  queue *q = create_queue(64);
  uint8_t visited[64] = {0};
  uint8_t cur;
  // mouse has visited vertex it is now at;
  //  visited[mouse.cur_pos] = 1;
  enqueue(q, mouse.cur_pos);

  while(q->size) {
    cur = q->elements[q->front];
    if (cur == cheese.cur_pos) {
      return; 
    }

    
  }
    
  free(queue);
  return path;
}

uint8_t * adj_to(uint8_t cur, uint8_t * adj) {
  /*
    returns vertices that are adjacent to cur
   */
  uint8_t * options;
  options = (typeof(options)) malloc(sizeof(* options) * 4);
  options = get_options(cur, options);
  for (int i = 0; i < 4; i++) {
      
  }
  free(options);
}

queue* create_queue(int maxElements) {
  /*
    Creates queue for use in bfs
   */
  queue *q;
  q = (queue*) malloc(sizeof(queue));
  
  q->elements = (int *) malloc(sizeof(int)*maxElements);
  q->size = 0;
  q->capacity = maxElements;
  q->front = 0;
  q->rear = -1;

  return q;
}

void dequeue(Queue q*) {
  /*
    dequeues an item.  To be used in bfs.
   */
  if(q->size == 0) {
    return;
  }
  else {
    q->size--;
    q->front++;

    // allow for circular filling of queue
    if(q->front == q->capacity) {
      q->front = 0;
    }
  }
  return;  
}

void enqueue(queue *q, uint8_t element) {
  // if queue is full, return
  if (q->size == q->capacity) {
    break;
  }
  else {
    q->size++;
    q->rear++;
    // allow for circular filling of queue
    if(q->rear == q->capacity) {
      q->rear = 0;
    }
    // insert element at rear
    q->elements[q->rear] = element;
  }
  return;
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
  random_cheese();
  draw_cheese();
  delay(500);
}
