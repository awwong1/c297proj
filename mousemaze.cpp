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
#include "map.h"
#include "mousemaze.h"
#include "mem_syms.h"
#include "lcd_image.h"
#include "bfs.h"
#include "queue.h"


// global variables go here
uint16_t joycenx;   // center value for x, should be around 512
uint16_t joyceny;   // center value for y, should be around 512
uint16_t num_walls = 0;
uint8_t pause;
entity mouse;
entity cheese;
wall wall_array[map_x * map_y][2];
point point_array[map_x * map_y];
point nullpoint;
uint8_t * path;
uint8_t path_len = 0; 
uint8_t trigger;
uint8_t refresh_cheese;


Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

uint8_t getSeed(){
  /*
    This function gets a pseudo random seed value for the random function
   */
  uint8_t seed = 0;
  for(int i = 0; i < 8; i++) {
    seed = seed << 1;
    seed = seed ^ analogRead(i);
  }
  return seed;
}

void initialize() {
  /*
    Initialize serial monitor, LED, Pause button, SD Card
    Fill the screen with black pixels
   */
  Serial.begin(9600);
  pinMode(nopthled, OUTPUT);
  digitalWrite(buttonpause, HIGH);

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
  /*
    Center the joystick, store the values into the global containers
   */

  pinMode(joypush, INPUT);
  digitalWrite(joypush, HIGH);
  joyceny = analogRead(joyver);
  joycenx = analogRead(joyhor);
}

void initialize_mouse() {
  /*
    Place the mouse in the top left corner of the map
   */
  mouse.prev_pos = null;
  mouse.cur_pos = mouse_start;
}

void initialize_cheese() {
  /*
    Place the cheese in the bottom right corner of the map
   */
  mouse.prev_pos = null;
  cheese.cur_pos = cheese_start;
}

void initialize_map(point * pointarray) {
  /*
    Initialize all points that will appear on the screen
   */
  
  int num_points = map_x * map_y;
  int count = 0;

  // Offset by 3 pixels to prevent left side off screen
  for (int y = 0; y < map_y; y++) {
    int y_coord =  (y * 15) + offset;
    for (int x = 0; x < map_x; x++) {
      int x_coord = (x * 15) + offset;
      point_array[count].x_coord = x_coord;
      point_array[count].y_coord = y_coord;
      if (DEBUG) {
	Serial.print("x: ");
	Serial.print(x);
	Serial.print(", y: ");
	Serial.print(y);
	Serial.print(", i: ");
	Serial.print(count);
	Serial.print(": ");
	Serial.print(point_array[count].x_coord);
	Serial.print(", ");
	Serial.println(point_array[count].y_coord);
      }
      count++;
    }
  }
  
  // This is placed here for visuals. Also, walls draw over corners currently.
  draw_corners(point_array);
}

void initialize_null_walls() {
  /*
    Make all the points of each wall equal to the nullpoint
    Make the nullpoint point to (0,0)
   */
  point nullpoint;
  nullpoint.x_coord = 0;
  nullpoint.y_coord = 0;
  for (uint8_t cellno = 0; cellno < map_x * map_y; cellno++) {
    for (uint8_t wallno = 0; wallno < 2; wallno++) {
      wall_array[cellno][wallno].pt1 = nullpoint;
      wall_array[cellno][wallno].pt2 = nullpoint;
    }
  }
}

void initialize_rand_walls(point *point_array) {
  /*
    Place walls randomly in map, walls must be placed in valid
    locations. No_walls does not have to have uniquely placed walls
   */

  uint8_t pt1, pt2;
  randomSeed(getSeed());
  uint8_t no_walls = random(wall_max);
  if (DEBUG) {
    Serial.print("Printing walls, Amount: ");
    Serial.println(no_walls);
  }
  for (uint8_t i = 0; i < no_walls; i++){
    uint8_t * options;
    options = (typeof(options)) malloc(sizeof(* options) * 2);
    if ( options == 0 ) {
      Serial.println("No memory!");
      while ( 1 ) {}
    }
    // Get a cell number
    pt1 = random(visible_pt + 1);
    while (pt1 % map_x == map_x - 1) {
      pt1 = random(visible_pt + 1);
    }

    options = get_options(pt1, options);
    uint8_t wallchoice;
    while(1) {
      // Get a wall number
      wallchoice = random(2);
      pt2 = options[wallchoice];
      if (DEBUG) {
	Serial.print("Wallchoice: ");
	Serial.print(wallchoice);
	Serial.print(" : ");
	Serial.println(options[wallchoice]);
      }
      if (pt2 != null) {
	break;
      }
    }
    
    // Assign the proper points to their wall array
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
    Get options takes a point 'pointvalue' and returns the viable
    neighbors of pointvalue. With our current design, this will return
    a point choice to the right of the point (+1), or below the
    point(+9)
   */
  uint8_t rightedge = 0;
  uint8_t botedge = 0;
  
  if (pointvalue % map_x == map_x - 1) {
    rightedge = 1;
  }
  if (pointvalue > visible_pt) {
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
      options[wallchoice] = pointvalue + map_x;
      botedge = 1;
      continue;
    }
    // No valid edge, put in the dummy point
    options[wallchoice] = null;
  }
  return options;
}

void print_all_walls() {
  /*
    Purely used for debugging purposes only.
    This prints all the walls, their x/y coords to the serial monitor
   */
  if (DEBUG) {
    for (uint8_t cell = 0; cell < map_x * map_y; cell++) {
      for (uint8_t wall = 0; wall < 2; wall++) {
	Serial.print("[");
	Serial.print(cell);
	Serial.print("][");
	Serial.print(wall);
	Serial.print("]: pt1(");
	Serial.print(wall_array[cell][wall].pt1.x_coord);
	Serial.print(", ");
	Serial.print(wall_array[cell][wall].pt1.y_coord);
	Serial.print("), pt2(");
	Serial.print(wall_array[cell][wall].pt2.x_coord);
	Serial.print(", ");
	Serial.print(wall_array[cell][wall].pt2.y_coord);
	Serial.println(")");
      }
    }
  }
  return;
}

void move_mouse_to(uint8_t pointlocation) {
  /*
    Takes the mouse and moves it to the location at the point location
    NOTE: This function does not check if pointlocation is valid for the mouse 
    ie) This function could move the mouse from any spot on
    the map to any spot on the map
   */
  if (pointlocation > visible_pt || pointlocation % map_x == map_x - 1) {
    // Invalid pointlocation
  }
  else {
    mouse.prev_pos = mouse.cur_pos;
    mouse.cur_pos = pointlocation;  
  }
  return;
}

void random_cheese() {
  /*
    Place the cheese in a random location on the map
    If the new location is the same as the mouse, 
    jump to a new location
   */
  while(1) {
    uint8_t location = random(visible_pt + 1);
    if (location > visible_pt || location % map_x == map_x - 1 || 
	location == mouse.cur_pos) {
      continue;
    }
    else {
      cheese.prev_pos = cheese.cur_pos;
      cheese.cur_pos = location;
      break;
    }
  }  
}

void draw_corners(point *point_array) {
  /*
    This function draws all the corners on the map as white pixels on
    the screen
   */
  for (int pixel = 0; pixel < map_x * map_y; pixel++) {
    tft.drawPixel(point_array[pixel].x_coord, point_array[pixel].y_coord, ST7735_WHITE);
  }
}

void draw_walls() {
  /*
    This function draws all the walls on the map as blue lines on the
    screen. Also draws one black pixel at the top left corner to
    account for the null walls
   */
  for (int cell = 0; cell < visible_pt; cell++) {
    for (int wall = 0; wall < 2; wall++) {     
      point apoint = wall_array[cell][wall].pt1;
      point bpoint = wall_array[cell][wall].pt2;
      
      tft.drawLine(apoint.x_coord, apoint.y_coord, bpoint.x_coord, bpoint.y_coord, ST7735_BLUE);
    }
  }
  tft.drawPixel(0, 0, ST7735_BLACK);
}

void draw_mouse(point *point_array) {
  /*
    This function removes an image at mouse.prev_pos
    and draws an image at mouse.cur_pos
   */
  uint8_t xval;
  uint8_t yval;
  if (mouse.prev_pos != null) {
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
  if (cheese.prev_pos != null) {
    xval = point_array[cheese.prev_pos].x_coord + 7;
    yval = point_array[cheese.prev_pos].y_coord + 7;
    tft.fillCircle(xval, yval, 4, ST7735_BLACK);
  }
  xval = point_array[cheese.cur_pos].x_coord + 7;
  yval = point_array[cheese.cur_pos].y_coord + 7;
  tft.fillCircle(xval, yval, 4, ST7735_YELLOW);
}

void drawtext(char *text) {
  /*
    This function displays white text below the maze on the lcd screen
   */
  tft.fillRect(0, 128, 128, 32, ST7735_BLACK);
  tft.setCursor(0, 128);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextWrap(true);
  tft.print(text);
}

uint8_t user_walls(){
  /*
    This function returns a boolean value (0 or 1) depending on yes or no
   */
  drawtext("Init random walls?");
  return yes_or_no();
}

uint8_t yes_or_no(){
  /*
    This function asks the user a yes or no question, 
    returning a boolean 0 or 1 based on the user answer
   */
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
    Else, if no input, selection == null
   */
  uint8_t selection = null;
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
  /*
    This function draws a corner selector on the map as a small circle
   */
  if (corner >= map_x * map_y) {
    return;
  }
  tft.drawCircle(point_array[corner].x_coord, point_array[corner].y_coord, 2, color);
  return;
}

void clear_corner_select(uint8_t corner, point *point_array) {
  /*
    This function draws a black corner selector on the map. Esentially this is 
    the same as draw_corner_select except the color is default black
   */
  if (corner >= map_x * map_y) {
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
  
  if (cell % map_x == map_x - 1) {
    drawtext("No extreme \nright edge \nallowed...");
    delay(1000);
    return;
  }
  if (cell > visible_pt) {
    drawtext("No extreme \nbottom edge \nallowed...");
    delay(1000);
    return;
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
    num_walls--;
  }
  else {
    wall_array[cell][wall].pt1 = point_array[cell];
    wall_array[cell][wall].pt2 = point_array[wallcorner];
    tft.drawLine(wall_array[cell][wall].pt1.x_coord, wall_array[cell][wall].pt1.y_coord, wall_array[cell][wall].pt2.x_coord, wall_array[cell][wall].pt2.y_coord, ST7735_BLUE);
    num_walls++;
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
  if (direction > offset) { return corner; }
  if (direction == 0) {
    if (corner > map_x - 1) { return (corner - map_x); }
    else { return corner; }
  }
  if (direction == 1) {
    if (corner % map_x != 0) { return (corner - 1); }
    else { return corner; }
  }
  if (direction == 2) {
    if (corner < visible_pt) { return (corner + map_x); }
    else { return corner; }
  }
  if (direction == offset) {
    if (corner % map_x != map_x - 1) { return (corner + 1); }
    else { return corner; }
  }
}

void setup() {
  path = (typeof(path)) malloc(sizeof(path) * map_x * map_y);
  initialize();
  initialize_joy();
  initialize_map(point_array);
  initialize_mouse();
  initialize_cheese();
  Serial.print("Cheese initialized to: ");
  Serial.println(cheese.cur_pos);
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
  refresh_cheese = 0;
}

void loop() {
  // Two different states; One for wall placement, One for mouse cycle
  // pause = 0; Simulate mouse finding cheese
  // pause = 1; Editor mode
  uint8_t cornerpos = null;
  trigger = 0;

  if (path_len == 0) { trigger = 1; }
  if (refresh_cheese) {
    if (DEBUG) {
      Serial.println("Cheese has been refreshed");
    }
    random_cheese();
    refresh_cheese = 0;
  }
  while (digitalRead(buttonpause) == 0) {
    if (!trigger){
      if (pause) { 
	pause = 0; 
	cornerpos = null;
      }
      else { 
	pause = 1; 
	cornerpos = 0;
      }
      trigger = 1;
    }
  }
  
  if (pause){
    if(trigger) {
      drawtext("Editor mode...");
      draw_corner_select(cornerpos, point_array, ST7735_RED);
      trigger = 0;
      // print_all_walls();
    }
    uint8_t edtrigger = 0;
    uint8_t joytrigger = 0;
    uint8_t direction = read_joy_input();
    uint8_t wallpos = 0;
    uint8_t walldirection = 0;
    uint8_t wedtrigger = 0;
    uint8_t wjoytrigger = 0;
    while(1) {
      if (edtrigger == 0 && joytrigger == 0) { direction = read_joy_input();}
      if (direction != null) { joytrigger = 1;}
      if (joytrigger == 1 && edtrigger == 0){
	clear_corner_select(cornerpos, point_array);
	cornerpos = move_to_corner(cornerpos, direction);
	draw_corner_select(cornerpos, point_array, ST7735_RED);
	edtrigger = 1;
      }
      if (edtrigger == 1 && joytrigger == 1) {
	if (read_joy_input() == null) {
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
	  if (walldirection != null) {
	    wjoytrigger = 1;
	  }
	  if (wjoytrigger == 1 && wedtrigger == 0){
	    clear_corner_select(wallpos, point_array);
	    wallpos = move_to_corner(cornerpos, walldirection);
	    draw_corner_select(wallpos, point_array, ST7735_GREEN);
	    wedtrigger = 1;
	  }
	  if (wedtrigger == 1 && wjoytrigger == 1) {
	    if (read_joy_input() == null) {
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
	      draw_corners(point_array);
	      drawtext("Editor mode...");
	      break;
	    }
	    else {
	      togglewall(cornerpos, wallpos, point_array);
	      clear_corner_select(wallpos, point_array);
	      draw_corners(point_array);
	      drawtext("Editor mode...");
	      break;
	    }
	  }
	  if (digitalRead(buttonpause) == 0) {
	    clear_corner_select(wallpos, point_array);
	    drawtext("Back...");
	    delay(1000);
	    drawtext("Editor mode...");
	    break;
	  }
	}
	draw_corner_select(cornerpos, point_array, ST7735_RED); 
      }
      if (digitalRead(buttonpause) == 0) {
	clear_corner_select(cornerpos, point_array);
	pause = 0;
	trigger = 1;
	break;
      }
    }
  }
  if (!pause) {
    if(trigger) {
      drawtext("Finding Path...");  
      Serial.println("Path function begins.");
      draw_cheese(point_array);
      draw_mouse(point_array);
      path_len = bfs(point_array, path, mouse, cheese);
      draw_mouse(point_array);
      Serial.println("Path function ends.");
      
      if (!path_len) {
	// LED ON
	digitalWrite(nopthled, HIGH);
	Serial.println("No path, cheese reset");
	drawtext("No path exists, \nrefresh cheese");
	draw_mouse(point_array);
	refresh_cheese = 1;
	path_len = 0;
	delay(1000);
      } 
      while (digitalRead(buttonpause) == 0) {
	pause = 1;
	cornerpos = 0;
	trigger = 1;
      }
    } 
    else {
      if (path_len < map_x * map_y) {
	if (DEBUG) {
	  Serial.print("Mouse has stepped: ");
	  Serial.println(path_len - 1);
	}
	// LED OFF
	digitalWrite(nopthled, LOW);
	drawtext("Simulating...");
	move_mouse_to(path[path_len-1]);
	draw_cheese(point_array);
	draw_mouse(point_array);
	draw_corners(point_array);
	path_len--;
	if (path_len-1 == 0) {
	  drawtext("Cheese found. Yum!");
	  delay(1000);
	  // Move the cheese to a new location
	  refresh_cheese = 1;
	}
	delay(500);
      }
      else if (path_len >= map_x * map_y) {
	// Shouldn't ever get here, but just in case
	drawtext("No path exists, \nrefresh cheese");
	refresh_cheese = 1;
	delay(1000);
      }
    }
  }
}
