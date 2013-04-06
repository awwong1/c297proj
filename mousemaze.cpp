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
#define buttonpause 12 // Pause button: Digital pin 12

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
uint8_t pause;
point * point_array;
entity mouse;
entity cheese;
wall wall_array[64][2];

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
void drawtext(char *text);

uint8_t user_walls();
uint8_t yes_or_no();


void setup();
void loop();
uint8_t * bfs(entity mouse, entity cheese);
uint8_t * adj_to(uint8_t cur, uint8_t * adj);
queue* create_queue(uint8_t maxElements);
void dequeue(queue *q);
void enqueue(queue *q, uint8_t element);


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
  mouse.prev_pos = 255;
  mouse.cur_pos = 0;
}

void initialize_cheese() {
  /*
    Place the cheese in the bottom right corner of the map
   */
  mouse.prev_pos = 255;
  cheese.cur_pos = 70;
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
  uint8_t pt1, pt2;
  randomSeed(getSeed());
  uint8_t no_walls = random(144);

  if (DEBUG) {
    Serial.print("Printing walls, Amount: ");
    Serial.println(no_walls);
  }
  
  // For every point 1, point 2 must be the x/y adjacent point
  // Usually can be +-1 or +-9, with the exception of the edges
  for (uint8_t i = 0; i < no_walls; i++){
    uint8_t * options;
    options = (typeof(options)) malloc(sizeof(* options) * 2);
    // always check the return code of malloc
    if ( options == 0 ) {
      Serial.println("No memory!");
      while ( 1 ) {}
    }
    // Choose a random cell from 0 to 63 inclusive, 
    // Of that cell, pick a random wall from 0 to 1 inclusive
    pt1 = random(63);
    options = get_options(pt1, options);
    uint8_t wallchoice;
    while(1) {
      wallchoice = random(2);
      pt2 = options[wallchoice];
      if (DEBUG) {
	Serial.print("Wallchoice: ");
	Serial.print(wallchoice);
	Serial.print(" : ");
	Serial.println(options[wallchoice]);
      }
      if (pt2 != 255) {
	break;
      }
    }
    
    wall_array[pt1][wallchoice].pt1 = point_array[pt1];
    wall_array[pt1][wallchoice].pt2 = point_array[pt2];
    
    free(options);

    if (DEBUG) {
      Serial.print("Wall Number: ");
      Serial.print(i);
      Serial.print(" Wall Points: (");
      Serial.print(wall_array[pt1][wallchoice].pt1.x_coord);
      Serial.print(", ");
      Serial.print(wall_array[pt1][wallchoice].pt1.y_coord);
      Serial.print("), (");
      Serial.print(wall_array[pt1][wallchoice].pt2.x_coord);
      Serial.print(", ");
      Serial.print(wall_array[pt1][wallchoice].pt2.y_coord);
      Serial.println(")");
    }
    num_walls++;
  }
}

uint8_t * get_options(uint8_t pointvalue, uint8_t * options){
  /*
    Get options takes a point 'pointvalue' and returns all of the
    viable neighbors of pointvalue. With our current design, this will
    return a point choice to the right of the point (+1), or below the
    point(+9)
   */
  uint8_t leftedge = 0;
  uint8_t rightedge = 0;
  uint8_t topedge = 0;
  uint8_t botedge = 0;

  // Set the fake boolean to true if edge case is true
  if (pointvalue < 9) { topedge = 1; }
  if (pointvalue > 71) { botedge = 1; }
  if ((pointvalue % 9) == 0) { leftedge = 1; }
  if ((pointvalue % 9) == 8) { rightedge = 1;}
    
  for (uint8_t wallchoice = 0; wallchoice < 2; wallchoice++) {  
    if (rightedge) {
      options[wallchoice] = 255;
    }
    if (!rightedge) {
      options[wallchoice] = pointvalue + 1;
      rightedge = 1;
      continue;
    }
    else if (!botedge) {
      options[wallchoice] = pointvalue + 9;
      botedge = 1;
      continue;
    }
    // dummy variable: 255 indicates non-existence
    options[wallchoice] = 255;
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
    if (location > 71 || location % 9 == 8 || location == mouse.cur_pos) {
      continue;
    }
    else {
      cheese.prev_pos = cheese.cur_pos;
      cheese.cur_pos = location;
      break;
    }
  }  
}

uint8_t* bfs(entity mouse, entity cheese) {
  /*
    performs a breadth first search from mouse to cheese
    returns array with path
   */
  queue *q = create_queue(64);
  uint8_t visited[64] = {0};
  uint8_t * adjacent;
  uint8_t cur;

  adjacent = (typeof(adjacent)) malloc(sizeof(* adjacent) * 4);
  // mouse has visited vertex it is now at;
  //  visited[mouse.cur_pos] = 1;
  enqueue(q, mouse.cur_pos);

  while(q->size) {
    cur = q->elements[q->front];
    if (cur == cheese.cur_pos) {
      return visited; 
    }
    // check neighbours
    //    adjacent = adj_to(cur, adj);
    /*
    for (int i = 0; i < ; i++) {
      
    }
    */
  }
  free(q);
  return visited;
}

uint8_t * adj_to(uint8_t cur, uint8_t * adj) {
  /*
    returns an array of vertices that are adjacent to cur
   */

  //  uint8_t * adj;
  adj = (typeof(adj)) malloc(sizeof(* adj) * 4);
  // check left vertex (cur's left wall)

  // check top vertex (cur's top wall)

  // check bottom vertex (cur + 9)'s top wall

  // check right vertex (cur + 1)'s left wall

  free(adj);
}

queue* create_queue(uint8_t maxElements) {
  /*
    Creates queue for use in bfs
   */
  queue *q;
  q = (queue*) malloc(sizeof(queue));
  
  q->elements = (uint8_t *) malloc(sizeof(int)*maxElements);
  q->size = 0;
  q->capacity = maxElements;
  q->front = 0;
  q->rear = -1;

  return q;
}

void dequeue(queue *q) {
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
    Serial.println("q at capacity");
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
  for (int cell = 0; cell < 64; cell++) {
    for (int wall = 0; wall < 2; wall++) {     
      point apoint = wall_array[cell][wall].pt1;
      point bpoint = wall_array[cell][wall].pt2;
      tft.drawLine(apoint.x_coord, apoint.y_coord, bpoint.x_coord, bpoint.y_coord, 0x0990);
    }
  }
}

void draw_mouse() {
  /*
    This function removes an image at mouse.prev_pos
    and draws an image at mouse.cur_pos
   */
  uint8_t xval;
  uint8_t yval;
  if (mouse.prev_pos != 255) {
    xval = point_array[mouse.prev_pos].x_coord + 7;
    yval = point_array[mouse.prev_pos].y_coord + 7;
    tft.fillCircle(xval, yval, 4, ST7735_BLACK);
  }
  xval = point_array[mouse.cur_pos].x_coord + 7;
  yval = point_array[mouse.cur_pos].y_coord + 7;
  tft.fillCircle(xval, yval, 4, 0xCCCC);
}

void draw_cheese() {
  /*
    This function removes an image at cheese.prev_pos
    and draws an image at cheese.cur_pos
   */  
  uint8_t xval;
  uint8_t yval;
  if (cheese.prev_pos != 255) {
    xval = point_array[cheese.prev_pos].x_coord + 7;
    yval = point_array[cheese.prev_pos].y_coord + 7;
    tft.fillCircle(xval, yval, 4, ST7735_BLACK);
  }
  xval = point_array[cheese.cur_pos].x_coord + 7;
  yval = point_array[cheese.cur_pos].y_coord + 7;
  tft.fillCircle(xval, yval, 4, ST7735_YELLOW);
}

void drawtext(char *text) {
  tft.fillRect(0, 128, 128, 32, ST7735_BLACK);
  tft.setCursor(0, 128);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextWrap(true);
  tft.print(text);
}

uint8_t user_walls(){
  drawtext("Init random walls?");
  return yes_or_no();
}

uint8_t yes_or_no(){
  tft.setCursor(78, 138);
  tft.print("N");
  tft.setCursor(26, 138);
  tft.print("Y");
  uint8_t selection = 1;
  while(1) {
    // Move cursor left
    if(analogRead(joyhor)>(joycenx+joyerr)) {
      selection = 1;
    }
    // Move cursor right
    if(analogRead(joyhor)<(joycenx-joyerr)) {
      selection = 0;
    }
    if (!selection) {
      tft.drawRect(76, 136, 9, 11, ST7735_MAGENTA);
      tft.drawRect(24, 136, 9, 11, ST7735_BLACK);
    }
    if (selection) {
      tft.drawRect(76, 136, 9, 11, ST7735_BLACK);
      tft.drawRect(24, 136, 9, 11, ST7735_MAGENTA);
    }
    if(digitalRead(joypush) == 0) {
      return selection;
    }
  }
}

void setup() {
  initialize();
  initialize_joy();
  point_array = initialize_map();
    
  initialize_mouse();
  initialize_cheese();  

  if (user_walls()) {
    drawtext("Initializing walls...");
    initialize_rand_walls();
    drawtext("Simulating...");
  } else {
    drawtext("Simulating...");
  }
  draw_walls();
  draw_corners();
  draw_mouse();
  draw_cheese();
}

void loop() {
  // Two different states; One for wall placement, One for mouse cycle
  // pause = 0; Simulate mouse finding cheese
  // pause = 1; Editor mode
  uint8_t trigger = 0;
  while (digitalRead(buttonpause) == 0) {
    if (!trigger){
      if (pause) {
	pause = 0;
      }
      else {
	pause = 1;
      }
    }
    trigger = 1;
  }
  
  if (pause){
    if(trigger) {
      drawtext("Editor mode...");
      trigger = 0;
    }
  }
  else if (!pause) {
    if(trigger) {
      drawtext("Simulating...");
      trigger = 0;
    }
    random_cheese();
    draw_cheese();
    draw_mouse();
  }
  trigger = 0;
}
