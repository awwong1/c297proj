#ifndef MAP_H
#define MAP_H

// defines the points used to draw edges, mouse, and cheese
typedef struct {
  uint8_t x_coord;
  uint8_t y_coord;
} point;


// defines the walls which have been drawn
typedef struct {
  point pt1;
  point pt2;
} wall;

// defines the mouse and cheese
// position is the top left hand vertex of the box
typedef struct {
  uint8_t prev_pos;
  uint8_t cur_pos;
} entity;

#endif
