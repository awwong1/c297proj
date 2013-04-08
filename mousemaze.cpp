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

// Necessary global variables go here
uint16_t joycenx;   // center value for x, should be around 512
uint16_t joyceny;   // center value for y, should be around 512
uint16_t num_walls = 0;
uint8_t pause;
point * point_array;
entity mouse;
entity cheese;
wall wall_array[65][2];

point nullpoint;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Forward declarations of project functions go here
uint8_t getSeed();
void initialize();
void initialize_joy();
void initialize_cheese();
point * initialize_map(point * point_array);
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
uint8_t * bfs(entity mouse, entity cheese);
uint8_t adj_to(uint8_t cur, uint8_t * adj);
bool is_equal(point pt1, point pt2);
queue* create_queue(uint8_t maxElements);
void dequeue(queue *q);
void enqueue(queue *q, uint8_t element);
bool membership (uint8_t * a_list, uint8_t len, uint8_t element);

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
  Serial.println("OK!");

  tft.fillScreen(tft.Color565(0x00, 0x00, 0x00));
}

void initialize_joy() {
  // Centers the joystick
  pinMode(joypush, INPUT);
  digitalWrite(joypush, HIGH);
  joyceny = analogRead(joyver);
  joycenx = analogRead(joyhor);
  // Initialize the pause button
  digitalWrite(buttonpause, HIGH);
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

point * initialize_map(point * pointarray) {
  /*
    Initialize all points that will appear on the screen
   */
  
  int num_points = map_x * map_y;
  int count = 0;
  
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
  
  if (DEBUG) {
    Serial.print("Points being initialized: ");
    for (int i = 0; i < num_points; i++) {
      Serial.print(i);
      Serial.print(": ");
      Serial.print(point_array[i].x_coord);
      Serial.print(", ");
      Serial.println(point_array[i].y_coord);
    }
  }

  // This is placed here for visuals. Also, walls draw over corners currently.
  draw_corners(point_array);
  return point_array;
}

void initialize_null_walls() {
  point nullpoint;
  nullpoint.x_coord = 0;
  nullpoint.y_coord = 0;
  for (uint8_t cellno = 0; cellno < 65; cellno++) {
    for (uint8_t wallno = 0; wallno < 2; wallno++) {
      wall_array[cellno][wallno].pt1 = nullpoint;
      wall_array[cellno][wallno].pt2 = nullpoint;
    }
  }
}

void initialize_rand_walls(point *point_array) {
  /*
    place walls randomly in map
   */

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
    
    while (pt1 % 9 == 8) {
      pt1 = random(63);
    }

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
      Serial.print(" Wall Coords: (");
      Serial.print(wall_array[pt1][wallchoice].pt1.x_coord);
      Serial.print(", ");
      Serial.print(wall_array[pt1][wallchoice].pt1.y_coord);
      Serial.print("), (");
      Serial.print(wall_array[pt1][wallchoice].pt2.x_coord);
      Serial.print(", ");
      Serial.print(wall_array[pt1][wallchoice].pt2.y_coord);
      Serial.println(")");
      Serial.print("Wall between points: ");
      Serial.print(pt1);
      Serial.print(", ");
      Serial.println(pt2);
    }
    num_walls++;
  }
}

uint8_t * get_options(uint8_t pointvalue, uint8_t* options){
  /*
    Get options takes a point 'pointvalue' and returns all of the
    viable neighbors of pointvalue. With our current design, this will
    return a point choice to the right of the point (+1), or below the
    point(+9)
   */
  uint8_t rightedge = 0;
  uint8_t botedge = 0;
  
  if (pointvalue % 9 == 8) {
    rightedge = 1;
  }
  if (pointvalue > 71) {
    botedge = 1;
  }
  
  for (uint8_t wallchoice = 0; wallchoice < 2; wallchoice++) {
    if (!rightedge) {
      // Not a right edge, therefore can add right neighbor
      options[wallchoice] = pointvalue + 1;
      rightedge = 1;
      continue;
    }
    if (!botedge) {
      // Not a bottom edge, therefore can add bottom neighbor
      options[wallchoice] = pointvalue + 9;
      botedge = 1;
      continue;
    }
    // No valid edge, put in the dummy point
    options[wallchoice] = 64;
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

uint8_t * bfs(point * point_array, uint8_t * visited, entity mouse, entity cheese) {
  /*
    performs a breadth first search from mouse to cheese
    returns array with path
  */
  uint8_t * adj;
  adj = (typeof(adj)) malloc(sizeof(* adj) * 4);
  queue * q = create_queue(64);
  uint8_t count = 0;
  uint8_t adj_len;
  uint8_t cur;
  // mouse has visited vertex it is now at;
  //  visited[mouse.cur_pos] = 1;
  enqueue(q, mouse.cur_pos);
  //  enqueue(visited, mouse.cur_pos);

  while(q->size) {
    cur = q->elements[q->front];
    Serial.print("current node: ");
    Serial.println(cur);
    dequeue(q);
    if (cur == cheese.cur_pos) {
      Serial.println("mouse has found the cheese");
      return visited;
    }
    // check neighbours
    adj_len = adj_to(cur, adj);
    for (int i = 0; i < adj_len; i++) {
      // if not in visited, add to queue
      if (membership(adj, adj_len, adj[i]) &&
	  membership(visited, count, adj[i]) == false) {
	visited[count] = cur;
	count++;
	enqueue(q, adj[i]);
      }
    }
    free(adj);
  }
  // TODO: free q
  Serial.print("No way to reach cheese");
  return (uint8_t*) NULL;
}

bool membership (uint8_t * a_list, uint8_t len, uint8_t element) {
  /*
    Tests if element is in a list.  Returns true or false.
   */
  bool in_list = false;
  for (int i = 0; i < len; i++) {
    if (a_list[i] == element) {
      in_list = true;
    }
  }
  return in_list;
}

uint8_t adj_to(uint8_t cur, uint8_t * adj) {
  /*
    returns vertices that are adjacent to cur
   */
  uint8_t count = 0;
  bool top = false;
  bool left = false;
  bool right = false;
  bool bottom = false;

  // check for edge cases
  if (cur < 9) {
    top = true;
  }
  if (cur > 72) {
    bottom = true;
  }
  if (cur % 9 == 8) {
    right = true;
  }
  if (cur % 9 == 0) {
    left = true;
  }

  // check left vertex (cur's left wall)
  if (is_equal(wall_array[cur][1].pt1, nullpoint) &&
      is_equal(wall_array[cur][1].pt2, nullpoint)) {
    adj[count] = cur - 1;
    if (left) {
      adj[count] = cur + 8;
    }
    count++;
  }
  // check top vertex (cur's top wall)
  if (is_equal(wall_array[cur][0].pt1, nullpoint) &&
      is_equal(wall_array[cur][0].pt2, nullpoint)) {
    adj[count] = cur - 9;
    if (top) {
      adj[count] = cur + 72;
    }
    count++;
  }
  // check bottom vertex (cur + 9)'s top wall
  if (is_equal(wall_array[cur + 9][0].pt1, nullpoint) &&
      is_equal(wall_array[cur + 9][0].pt2, nullpoint)) {
    adj[count] = cur + 9;
    if (bottom) {
      adj[count] = cur - 72;
    }
    count++;
  }
  // check right vertex (cur + 1)'s left wall
  if (is_equal(wall_array[cur + 1][1].pt1, nullpoint) &&
      is_equal(wall_array[cur + 1][1].pt2, nullpoint)) {
    adj[count] = cur + 1;
    if (right) {
      adj[count] = cur - 8;
    }
    count++;
  }

  if (DEBUG) {
    Serial.print("adjacent cells: ");
    for (int i = 0; i < count; i++) {
      Serial.print(adj[i]);
      Serial.print(", ");
    }
    Serial.println("");
  }
  return count;
}

bool is_equal(point pt1, point pt2) {
  /*
    Tests if two points are equal.  Returns true/false.
   */
  bool equal = false;
  if (pt1.x_coord == pt2.x_coord && pt1.y_coord == pt2.y_coord) {
    equal = true;
  }
  return equal;
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

void draw_corners(point *point_array) {
  for (int pixel = 0; pixel < 81; pixel++) {
    tft.drawPixel(point_array[pixel].x_coord, point_array[pixel].y_coord, ST7735_WHITE);
  }
}

void draw_walls() {
  for (int cell = 0; cell < 64; cell++) {
    for (int wall = 0; wall < 2; wall++) {     
      point apoint = wall_array[cell][wall].pt1;
      point bpoint = wall_array[cell][wall].pt2;
      
      tft.drawLine(apoint.x_coord, apoint.y_coord, bpoint.x_coord, bpoint.y_coord, ST7735_BLUE);
    }
  }
}

void draw_mouse(point *point_array) {
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

void draw_cheese(point *point_array) {
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

uint8_t read_joy_input() {
  /*
    This function reads returns the left, right, up, down value of the
    joypad, where 0: up, 1: left, 2: down, 3: right
    Else, if no input, selection == 255
   */
  uint8_t selection = 255;
  // Move cursor left
  if(analogRead(joyhor)>(joycenx+joyerr)) {
    selection = 1;
  }
  // Move cursor right
  if(analogRead(joyhor)<(joycenx-joyerr)) {
    selection = 3;
  }
  // Move cursor up
  if(analogRead(joyver)>(joyceny+joyerr)) {
    selection = 0;
  }
  if(analogRead(joyver)<(joyceny-joyerr)) {
    selection = 2;
  }
  return selection;
}

void draw_corner_select(uint8_t corner, point *point_array, uint16_t color) {
  if (corner >= 81) {
    return;
  }
  tft.drawCircle(point_array[corner].x_coord, point_array[corner].y_coord, 2, color);
  return;
}

void clear_corner_select(uint8_t corner, point *point_array) {
  if (corner >= 81) {
    return;
  }
  tft.drawCircle(point_array[corner].x_coord, point_array[corner].y_coord, 2, ST7735_BLACK);
  return;
}

void togglewall(uint8_t corner1, uint8_t corner2, point *point_array) {
  /*
    This function takes two points, corner1 and corner2 and checks if
    a wall exists between those points. If the wall exists, this
    function will remove the wall in question, else it will create a
    wall between those two points.
    
  */
  
  // Find the minimum corner
  uint8_t cell;
  uint8_t wallcorner;
  uint8_t wall;
  if (corner1 == corner2) {
    drawtext("Invalid wall, same point");
    delay(1000);
    return;
  }
  if (corner1 < corner2) {
    cell = corner1;
    wallcorner = corner2;
  }
  else {
    cell = corner2;
    wallcorner = corner1;
  }
  if (wallcorner == (cell + 1)) {
    wall = 0;
  }
  else {
    wall = 1;
  }
  if (DEBUG) {
    Serial.print("Cell Number: ");
    Serial.print(cell);
    Serial.print(", Wall Corner Number: ");
    Serial.println(wallcorner);
  }

  // check if a wall exists
  if (DEBUG) {
    Serial.println("Map Wall Struct Values: ");
    Serial.print("Cell Number = ");
    Serial.println(cell);
    Serial.print("WALLARRAY Point 1 Coordinates: (");
    Serial.print(wall_array[cell][wall].pt1.x_coord);
    Serial.print(", ");
    Serial.print(wall_array[cell][wall].pt1.y_coord);
    Serial.println(")");
    Serial.print("WALLARRAY Point 2 Coordinates: (");
    Serial.print(wall_array[cell][wall].pt2.x_coord);
    Serial.print(", ");
    Serial.print(wall_array[cell][wall].pt2.y_coord);
    Serial.println(")");
    Serial.print("POINTARRAY Point 1 Coordinates: (");
    Serial.print(point_array[cell].x_coord);
    Serial.print(", ");
    Serial.print(point_array[cell].y_coord);
    Serial.println(")");
    Serial.print("POINTARRAY Point 2 Coordinates: (");
    Serial.print(point_array[wallcorner].x_coord);
    Serial.print(", ");
    Serial.print(point_array[wallcorner].y_coord);
    Serial.println(")");
  }
  if (point_array[cell].x_coord == wall_array[cell][wall].pt1.x_coord && point_array[cell].y_coord == wall_array[cell][wall].pt1.y_coord && point_array[wallcorner].x_coord == wall_array[cell][wall].pt2.x_coord && point_array[wallcorner].y_coord == wall_array[cell][wall].pt2.y_coord) {
    // Toggle off
    point nullpoint;
    nullpoint.x_coord = 0;
    nullpoint.y_coord = 0;
    wall_array[cell][wall].pt1 = nullpoint;
    wall_array[cell][wall].pt2 = nullpoint;
    tft.drawLine(point_array[cell].x_coord, point_array[cell].y_coord, point_array[wallcorner].x_coord, point_array[wallcorner].y_coord, ST7735_BLACK);
  }
  else {
    wall_array[cell][wall].pt1 = point_array[cell];
    wall_array[cell][wall].pt2 = point_array[wallcorner];
    tft.drawLine(wall_array[cell][wall].pt1.x_coord, wall_array[cell][wall].pt1.y_coord, wall_array[cell][wall].pt2.x_coord, wall_array[cell][wall].pt2.y_coord, ST7735_BLUE);
  }
  drawtext("Wall has been toggled");
  delay(1000);
  return;
}

uint8_t move_to_corner(uint8_t corner, uint8_t direction) {
  /*
    Checks to see if the supplied direction is valid for the given
    corner, then returns the resulting corner, if the direction is
    invalid, the corner does not move
   */
  if (direction > 3) {
    return corner;
  }
  if (direction == 0) {
    if (corner > 8) {
      return (corner - 9);
    }
    else {
      return corner;
    }
  }
  if (direction == 1) {
    if (corner % 9 != 0) {
      return (corner - 1);
    }
    else {
      return corner;
    }
  }
  if (direction == 2) {
    if (corner < 72) {
      return (corner + 9);
    }
    else {
      return corner;
    }
  }
  if (direction == 3) {
    if (corner % 9 != 8) {
      return (corner + 1);
    }
    else {
      return corner;
    }
  }
}

void setup() {
  initialize();
  initialize_joy();
  
  point_array = (typeof(point_array)) malloc(sizeof(*point_array) * map_x * map_y);
  if (point_array == 0) {
    Serial.println("No Memory");
    while (1) {}
  }

  point_array = initialize_map(point_array);
  
  initialize_mouse();
  initialize_cheese();
  initialize_null_walls();
  draw_corners(point_array);

  if (user_walls()) {
    drawtext("Initializing walls...");
    initialize_rand_walls(point_array);
    Serial.println("Moving to drawtext('Simulating...');");
    drawtext("Simulating...");
  } else {
    drawtext("Simulating...");
  }
  draw_walls();
  draw_mouse(point_array);
  draw_cheese(point_array);
  pause = 0;
}

void loop() {
  // Two different states; One for wall placement, One for mouse cycle
  // pause = 0; Simulate mouse finding cheese
  // pause = 1; Editor mode
  uint8_t trigger = 0;
  uint8_t cornerpos = 255;

  while (digitalRead(buttonpause) == 0) {
    if (!trigger){
      if (pause) { 
	pause = 0; 
	cornerpos = 255;
      }
      else { 
	pause = 1; 
	cornerpos = 0;
      }
    }
    trigger = 1;
  }
  
  if (pause){
    if(trigger) {
      drawtext("Editor mode...");
      draw_corner_select(cornerpos, point_array, ST7735_RED);
      trigger = 0;
    }
    uint8_t edtrigger = 0;
    uint8_t joytrigger = 0;
    uint8_t direction = read_joy_input();
    uint8_t wallpos = 0;
    uint8_t walldirection = 0;
    uint8_t wedtrigger = 0;
    uint8_t wjoytrigger = 0;
    while(1) {
      if (edtrigger == 0 && joytrigger == 0) {
	direction = read_joy_input();
      }
      if (direction != 255) {
	joytrigger = 1;
      }
      if (joytrigger == 1 && edtrigger == 0){
	clear_corner_select(cornerpos, point_array);
	cornerpos = move_to_corner(cornerpos, direction);
	draw_corner_select(cornerpos, point_array, ST7735_RED);
	edtrigger = 1;
      }
      if (edtrigger == 1 && joytrigger == 1) {
	if (read_joy_input() == 255) {
	  edtrigger = 0;
	  joytrigger = 0;
	}
      }
      if (digitalRead(joypush) == 0) {
	// Corner selected
	wallpos = cornerpos;
	drawtext("Select next corner");
	delay(500);
	while(1) {
	  draw_corner_select(cornerpos, point_array, ST7735_RED);
	  if (wedtrigger == 0 && wjoytrigger == 0) {
	    walldirection = read_joy_input();
	  }
	  if (walldirection != 255) {
	    wjoytrigger = 1;
	  }
	  if (wjoytrigger == 1 && wedtrigger == 0){
	    clear_corner_select(wallpos, point_array);
	    wallpos = move_to_corner(cornerpos, walldirection);
	    draw_corner_select(wallpos, point_array, ST7735_GREEN);
	    wedtrigger = 1;
	  }
	  if (wedtrigger == 1 && wjoytrigger == 1) {
	    if (read_joy_input() == 255) {
	      wedtrigger = 0;
	      wjoytrigger = 0;
	    }
	  }
	  if (digitalRead(joypush) == 0) {
	    // Second point has been selected, add the wall, draw the
	    // wall and break
	    if (wallpos == cornerpos) {
	      drawtext("Same point, invalid");
	      delay(1000);
	      clear_corner_select(wallpos, point_array);
	      drawtext("Editor mode...");
	      break;
	    }
	    else {
	      togglewall(cornerpos, wallpos, point_array);
	      clear_corner_select(wallpos, point_array);
	      drawtext("Editor mode...");
	      break;
	    }
	  }
	  if (digitalRead(buttonpause) == 0) {
	    clear_corner_select(wallpos, point_array);
	    drawtext("Editor mode...");
	    break;
	  }
	}
	draw_corner_select(cornerpos, point_array, ST7735_RED); 
      }
      if (digitalRead(buttonpause) == 0) {
	clear_corner_select(cornerpos, point_array);
	break;
      }
    }
  }
  else if (!pause) {
    if(trigger) {
      drawtext("Simulating...");
      trigger = 0;
    }
    random_cheese();
    draw_cheese(point_array);
    draw_mouse(point_array);
    delay(100);
  }
  draw_corners(point_array);
  // uint8_t * visited;
  // visited = (typeof(visited)) malloc(sizeof(* visited) * 81);
  // visited = bfs(point_array, visited, mouse, cheese);
  
  /*
  if (!visited) {
    Serial.println("No path, reinitialize walls");
    initialize_null_walls();
    initialize_rand_walls(point_array);
    draw_walls();
  }
  */

  trigger = 0;
}
