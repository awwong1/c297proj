#ifndef MAP_H
#define MAP_H

// defines the points used to draw edges, mouse, and cheese
typedef struct {
  uint8_t x_coord;
  uint8_t y_coord;
} point;


// defines the edges which have been drawn
typedef struct {
  point pt1;
  point pt2;
} edge;


#endif
