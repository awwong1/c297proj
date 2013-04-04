#ifndef MAP_H
#define MAP_H

// defines the points used to draw edges, mouse, and cheese
typedef struct {
  uint8_t x_coord;
  uint8_t y_coord;
  uint8_t point_id;
  } point;


// defines the edges which have been drawn
/*  DOESN'T WORK.  WHY?? expected constructor, destructor, or 
    type conversion before... error

typdef struct {
  point pt1;
  point pt2;
} edge;

*/

#endif
