#include <Arduino.h>
#include "map.h"
#include "mousemaze.h"
#include "bfs.h"
#include "mem_syms.h"
#include "queue.h"

extern wall wall_array[map_x * map_y][2];
extern point point_array[map_x * map_y];
extern point nullpoint;

uint8_t bfs(point * point_array, uint8_t * path, entity mouse, entity cheese) {
  /*
    performs a breadth first search from mouse to cheese
    returns array with path
  */

  // list of neighbours of a point that is not blocked by wall
  uint8_t * adj;
  adj = (typeof(adj)) malloc(sizeof(* adj) * 4);
  
  if (adj == 0) {
    Serial.println("adj Outta Memory!");
    while(1) {}
  }

  queue * q = create_queue(visible_pt);
  uint8_t count = 0;
  uint8_t adj_len;
  uint8_t cur_point;
  node cur_node;
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
    // take first in queue
    cur_point = q->elements[q->front];
    dequeue(q);

    Serial.print("current node: ");
    Serial.println(cur_point);
    
    // if cur_point == dest, we are done
    if (cur_point == cheese.cur_pos) {
      Serial.println("mouse has found the cheese");
      // extract out the path
      path_len = extract_path(visited, count, path, cheese.cur_pos, mouse.cur_pos);
      free(q->elements);
      free(q);
      free(visited);
      free(adj);
      return path_len;
    }
    // check neighbours
    adj_len = adj_to(cur_point, adj);
    for (int i = 0; i < adj_len; i++) {
      // if not in queue or visited, add to queue and visited
      if (membership_visited(visited, count, adj[i]) == false &&
	  membership_queue(q, q->size, adj[i]) == false) {
	// when adding cur_point to visited, set parent as well
	cur_node.parent = cur_point;
	cur_node.pt = adj[i];
	visited[count] = cur_node;
	count++;
	// enqueue the adjacent points
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

uint8_t adj_to(uint8_t cur, uint8_t * adj) {
  /*
    returns points that are adjacent to cur
    stores in * adj and returns no. of elements
   */
  uint8_t count = 0;
  bool top = false;
  bool left = false;
  bool right = false;
  bool bottom = false;

  // check to see if on edges of map
  if (cur < map_x) {
    top = true;
    if (DEBUG) {
      Serial.println("top = true");
    }
  }
  if (cur > visible_pt - map_x) {
    bottom = true;
    if (DEBUG) {
      Serial.println("bottom = true");
    }

  }
  if (cur % map_x == map_x - 2) {
    right = true;
    if (DEBUG) {
      Serial.println("right = true");
    }

  }
  if (cur % map_x == 0) {
    left = true;
    if (DEBUG) {
      Serial.println("left = true");
    }

  }

  // check left vertex (cur's left wall)
  if (is_equal(wall_array[cur][1].pt1, nullpoint) &&
      is_equal(wall_array[cur][1].pt2, nullpoint)) {
    adj[count] = cur - 1;
    if (left) {
      adj[count] = cur + map_x - 2;
    }
    count++;
  }
  // check top vertex (cur's top wall)
  if (is_equal(wall_array[cur][0].pt1, nullpoint) &&
      is_equal(wall_array[cur][0].pt2, nullpoint)) {
    adj[count] = cur - map_x;
    if (top) {
      adj[count] = cur + visible_pt - map_x + 1;
    }
    count++;
  }
  // check bottom vertex (cur + 9)'s top wall
  if (!bottom && 
      is_equal(wall_array[cur + 9][0].pt1, nullpoint) &&
      is_equal(wall_array[cur + 9][0].pt2, nullpoint)) {
    adj[count] = cur + map_x;
    count++;
  }

  // if cur is on bottom row, bottom's top row is actually first row
  if (bottom && 
      is_equal(wall_array[cur - 63][0].pt1, nullpoint) &&
      is_equal(wall_array[cur - 63][0].pt2, nullpoint)) {
    adj[count] = cur - (visible_pt - map_x + 1);
    count++;
  }
  
  // check right vertex (cur + 1)'s left wall
  if (!right && 
      is_equal(wall_array[cur + 1][1].pt1, nullpoint) &&
      is_equal(wall_array[cur + 1][1].pt2, nullpoint)) {
    adj[count] = cur + 1;
    count++;
  }
  // if cur is on right column, right's left wall is on first column
  if (right && 
      is_equal(wall_array[cur - 7][1].pt1, nullpoint) &&
      is_equal(wall_array[cur - 7][1].pt2, nullpoint)) {
    adj[count] = cur - (map_x - 2);
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


uint8_t extract_path(node * visited, uint8_t len, uint8_t * path, uint8_t dest, uint8_t source) {
  /*
    extracts path from visited list.  Returns no. of elements
    Note: path will be in reverse order
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
    // find point's parent, append to path
    for (int i = 0; i < len; i++) {
      if (visited[i].pt == path[count - 1]) {
	path[count] = visited[i].parent;
	count++;
	break;
      }
    }
  }
  Serial.print("Path is: ");
  for (int i = 0; i < count; i++) {
    Serial.print(path[i]);
    Serial.print(", ");
  }
  Serial.println("");

  return count;
}
