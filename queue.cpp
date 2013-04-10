#include <Arduino.h>
#include "queue.h"

queue * create_queue(uint8_t maxElements) {
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
