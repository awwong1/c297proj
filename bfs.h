#ifndef BFS_H
#define BFS_H

typedef struct {
  uint8_t pt;
  uint8_t parent;
} node;

uint8_t bfs(point * point_array, uint8_t * path, entity mouse, entity cheese);
  /*
    performs a breadth first search from mouse to cheese
    returns array with path
  */

uint8_t adj_to(uint8_t cur, uint8_t * adj);
  /*
    returns vertices that are adjacent to cur
    stores in * adj and returns no. of elements
   */

bool is_equal(point pt1, point pt2);
  /*
    Tests if two points are equal.  Returns true/false.
   */

bool membership_visited (node * a_list, uint8_t len, uint8_t element);
  /*
    Tests if element is in a list of nodes.  Returns true or false.
   */

uint8_t extract_path(node * visited, uint8_t len, uint8_t * path, uint8_t dest, uint8_t source);
  /*
    extracts path from visited list.  Returns no. of elements
   */

#endif
