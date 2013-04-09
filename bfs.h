#ifndef BFS_H
#define BFS_H


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

queue* create_queue(uint8_t maxElements);
  /*
    Creates queue for use in bfs
   */

void dequeue(queue *q);
  /*
    dequeues an item.  To be used in bfs.
   */

void enqueue(queue *q, uint8_t element);
  /*
    enqueues an item. To be used in bfs.
   */

bool membership_visited (node * a_list, uint8_t len, uint8_t element);
  /*
    Tests if element is in a list of nodes.  Returns true or false.
   */


bool membership_queue (queue * a_list, uint8_t len, uint8_t element);
  /*
    Tests if element is in a queue of elements.  Returns true or false.
   */

uint8_t extract_path(node * visited, uint8_t len, uint8_t * path, uint8_t dest);
  /*
    extracts path from visited list.  Returns no. of elements
   */

#endif
