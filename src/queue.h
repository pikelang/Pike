#ifndef QUEUE_H
#define QUEUE_H

struct queue;
typedef void (*queue_call)(void *data);

/* Prototypes begin here */
struct queue_entry;
struct queue_block;
void run_queue(struct queue *q);
void enqueue(struct queue *q, queue_call call, void *data);
/* Prototypes end here */

struct queue
{
  struct queue_block *first, *last;
};


#endif /* QUEUE_H */
