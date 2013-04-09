#include <Arduino.h>
#include "map.h"
#include "bfs.h"
#include "mem_syms.h"

extern wall wall_array[81][2];
extern point point_array[81];
extern point nullpoint;

#define DEBUG    1      // Set this to 1 for debug serial printing

uint8_t bfs(point * point_array, uint8_t * path, entity mouse, entity cheese) {
  /*
    performs a breadth first search from mouse to cheese
    returns array with path
  */

  uint8_t * adj;
  adj = (typeof(adj)) malloc(sizeof(* adj) * 4);
  
  if (adj == 0) {
    Serial.println("adj Outta Memory!");
    while(1) {}
  } else {
    Serial.println("adj ok");
    Serial.println(AVAIL_MEM);
    Serial.println(STACK_SIZE);
  }

  queue * q = create_queue(64);
  uint8_t count = 0;
  uint8_t adj_len;
  uint8_t cur_pt;
  node cur;
  node * visited;
  uint8_t path_len;
  visited = (typeof(visited)) malloc(sizeof(* visited) * 81);

  if (visited == 0) {
    Serial.println("Visited Outta Memory!");
    while(1) {}
  } else {
    Serial.println("Visited ok");
  }


  Serial.print("mouse is at: ");
  Serial.println(mouse.cur_pos);
  Serial.print("cheese is at: ");
  Serial.println(cheese.cur_pos);

  // mouse has visited vertex it is now at;
  enqueue(q, mouse.cur_pos);

  while(q->size) {
    cur_pt = q->elements[q->front];
    Serial.print("current node: ");
    Serial.println(cur_pt);
    dequeue(q);
    if (cur_pt == cheese.cur_pos) {
      Serial.println("mouse has found the cheese");
      path_len = extract_path(visited, count, path, cheese.cur_pos, mouse.cur_pos);
      free(q->elements);
      free(q);
      free(visited);
      free(adj);
      return path_len;
    }
    // check neighbours
    adj_len = adj_to(cur_pt, adj);
    for (int i = 0; i < adj_len; i++) {
      // if not in visited, add to queue
      if (membership_visited(visited, count, adj[i]) == false &&
	  membership_queue(q, q->size, adj[i]) == false) {
	cur.parent = cur_pt;
	cur.pt = adj[i];
	visited[count] = cur;
	count++;
	enqueue(q, adj[i]);
      }
    }
  }
  Serial.print("No way to reach cheese");
  free(q->elements);
  free(adj);
  free(q);
  free(visited);
  return (uint8_t) NULL;
}

bool membership_visited (node * a_list, uint8_t len, uint8_t element) {
  /*
    Tests if element is in a list of nodes.  Returns true or false.
   */
  bool in_list = false;
  for (int i = 0; i < len; i++) {
    if (a_list[i].pt == element) {
      in_list = true;
    }
  }
  return in_list;
}

bool membership_queue (queue * a_list, uint8_t len, uint8_t element) {
  /*
    Tests if element is in a queue of elements.  Returns true or false.
   */
  bool in_list = false;
  for (int i = 0; i < len; i++) {
    if (a_list->elements[i] == element) {
      in_list = true;
    }
  }
  return in_list;
}

uint8_t adj_to(uint8_t cur, uint8_t * adj) {
  /*
    returns vertices that are adjacent to cur
    stores in * adj and returns no. of elements
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
  if (cur > 62) {
    bottom = true;
  }
  if (cur % 9 == 7) {
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
      adj[count] = cur + 7;
    }
    count++;
  }
  // check top vertex (cur's top wall)
  if (is_equal(wall_array[cur][0].pt1, nullpoint) &&
      is_equal(wall_array[cur][0].pt2, nullpoint)) {
    adj[count] = cur - 9;
    if (top) {
      adj[count] = cur + 63;
    }
    count++;
  }
  // check bottom vertex (cur + 9)'s top wall
  if (is_equal(wall_array[cur + 9][0].pt1, nullpoint) &&
      is_equal(wall_array[cur + 9][0].pt2, nullpoint)) {
    adj[count] = cur + 9;
    if (bottom) {
      adj[count] = cur - 63;
    }
    count++;
  }
  // check right vertex (cur + 1)'s left wall
  if (is_equal(wall_array[cur + 1][1].pt1, nullpoint) &&
      is_equal(wall_array[cur + 1][1].pt2, nullpoint)) {
    adj[count] = cur + 1;
    if (right) {
      adj[count] = cur - 7;
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
  /*
    enqueues an item. To be used in bfs.
   */

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

uint8_t extract_path(node * visited, uint8_t len, uint8_t * path, uint8_t dest, uint8_t source) {
  /*
    extracts path from visited list.  Returns no. of elements
   */
  uint8_t count = 1;
  if (DEBUG) {
    Serial.print("Parent list is: ");
    for (int i = 0; i < len; i++) {
      Serial.print(visited[i].pt);
      Serial.print(": ");
      Serial.print(visited[i].parent);
      Serial.print(", ");
    }
    Serial.println("");
  }
  path[0] = dest;
  // while not at mouse starting position = 0
  while (path[count-1] != source) {
    Serial.print("path[count-1]: ");
    Serial.println(path[count-1]);
    for (int i = 0; i < len; i++) {
      if (visited[i].pt == path[count - 1]) {
	path[count] = visited[i].parent;
	count++;
	break;
      }
    }
  }
  if (DEBUG) {
    Serial.print("Path is: ");
    for (int i = 0; i < count; i++) {
      Serial.print(path[i]);
      Serial.print(", ");
    }
    Serial.println("");
  }
  return count;
}
