#ifndef MAP_H
#define MAP_H

// defines the points used to draw edges, mouse, and cheese
typedef struct {
  uint8_t x_coord;
  uint8_t y_coord;
  int num;
} point;


// defines the walls which have been drawn
typedef struct {
  int num;
  point pt1;
  point pt2;
} wall;


#endif
