#include "global.h"
#include "pike_macros.h"
#include "queue.h"

struct queue_entry
{
  queue_call call;
  void *data;
};

#define QUEUE_ENTRIES 8191

struct queue_block
{
  struct queue_block *next;
  int used;
  struct queue_entry entries[QUEUE_ENTRIES];
};

void run_queue(struct pike_queue *q)
{
  struct queue_block *b;
  while((b=q->first))
  {
    int e;
    for(e=0;e<b->used;e++)
    {
      debug_malloc_touch(b->entries[e].data);
      b->entries[e].call(b->entries[e].data);
    }

    q->first=b->next;
    free((char *)b);
  }
  q->last=0;
}

void enqueue(struct pike_queue *q, queue_call call, void *data)
{
  struct queue_block *b=q->last;
  if(!b || b->used >= QUEUE_ENTRIES)
  {
    b=ALLOC_STRUCT(queue_block);
    b->used=0;
    b->next=0;
    if(q->first)
      q->last->next=b;
    else
      q->first=b;
    q->last=b;
  }

  b->entries[b->used].call=call;
  b->entries[b->used].data=debug_malloc_pass(data);
  b->used++;
}
