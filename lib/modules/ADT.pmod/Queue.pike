// $Id: Queue.pike,v 1.11 2004/06/18 09:13:51 grubba Exp $

//! A simple FIFO queue.

#pike __REAL_VERSION__
#define QUEUE_SIZE 100

array l;
int head;
int tail;

//! Creates a queue with the initial items @[args] in it.
static void create(mixed ...args)
{
  l = args + allocate(QUEUE_SIZE);
  head = sizeof(args);
  tail = 0;
}

static int _sizeof()
{
  return head - tail;
}

static array _values()
{
  return l[tail..head];
}

void write(mixed item)
{
  put(item);
}

//! @decl void write(mixed item)
//! @decl void put(mixed item)
//! Adds the @[item] to the queue.
//
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

mixed read()
{
  return get();
}

//! @decl mixed read()
//! @decl mixed get()
//! Returns the next element from the queue.
//
mixed get()
{
//  werror(sprintf("Queue->get: %O\n", l[tail..head-1]));
  mixed res;
  if (tail == head)
    return UNDEFINED;
  res = l[tail];
  l[tail++] = 0;
  return res;
}

//! Returns the next element from the queue
//! without removing it from the queue.
mixed peek()
{
  return (tail < head) && l[tail];
}

//! Returns true if the queue is empty,
//! otherwise zero.
int(0..1) is_empty()
{
  return (tail == head);
}

//! Empties the queue.
void flush()
{
  create();
}

//! It is possible to cast ADT.Queue to an array.
static mixed cast(string to) {
  switch(to) {
  case "object": return this;
  case "array": return l[tail..head-1];
  }
  error("Can not cast ADT.Queue to %s.\n", to);
}

static string _sprintf(int t) {
  return t=='O' && sprintf("%O%O", this_program, cast("array"));
}
