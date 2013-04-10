#ifndef QUEUE_H
#define QUEUE_H

typedef struct {
  uint8_t capacity;
  uint8_t size;
  uint8_t front;
  uint8_t rear;
  uint8_t *elements;
} queue;

queue * create_queue(uint8_t maxElements);
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

bool membership_queue (queue * a_list, uint8_t len, uint8_t element);
  /*
    Tests if element is in a queue of elements.  Returns true or false.
   */

#endif
