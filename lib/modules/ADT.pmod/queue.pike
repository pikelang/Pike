/* $Id: queue.pike,v 1.5 1997/05/31 22:03:44 grubba Exp $
 *
 * A simple FIFO queue. 
 */

#define QUEUE_SIZE 100

array l;
int head;
int tail;

void create(mixed ...args)
{
  l = args + allocate(QUEUE_SIZE);
  head = sizeof(args);
  tail = 0;
}

void put(mixed item)
{
  if (head == sizeof(l))
  {
    l = l[tail ..];
    head -= tail;
    tail = 0;
    l += allocate(sizeof(l) + QUEUE_SIZE);
  }
  l[head++] = item;
//  werror(sprintf("Queue->put: %O\n", l[tail..head-1]));
}

mixed get()
{
//  werror(sprintf("Queue->get: %O\n", l[tail..head-1]));
  mixed res;
  if (tail == head)
    return 0;
  res = l[tail];
  l[tail++] = 0;
  return res;
}

mixed peek()
{
  return (tail < head) && l[tail];
}

int is_empty()
{
  return (tail == head);
}

void flush()
{
  create();
}
