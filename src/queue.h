#ifndef QUEUE_H
#define QUEUE_H

struct pike_queue
{
  struct queue_block *first, *last;
};

typedef void (*queue_call)(void *data);

/* Prototypes begin here */
struct queue_entry;
struct queue_block;
void run_queue(struct pike_queue *q);
void enqueue(struct pike_queue *q, queue_call call, void *data);
/* Prototypes end here */

#endif /* QUEUE_H */
