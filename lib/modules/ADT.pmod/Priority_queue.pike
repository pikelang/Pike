#pike __REAL_VERSION__

inherit .Heap;

//! This class implements a priority queue. Each element in the priority
//! queue is assigned a priority value, and the priority queue always
//! remains sorted in increasing order of the priority values. The top of
//! the priority queue always holds the element with the smallest priority.
//! The priority queue is realized as a (min-)heap.

class elem {
  inherit Element;

  int|float pri;

  protected void create(int|float a, mixed b) { pri=a; ::create(b); }

  void set_pri(int|float p)
    {
      pri=p;
      adjust(this);
    }

  int|float get_pri() { return pri; }

  protected int `<(object o) { return pri<o->pri; }
  protected int `>(object o) { return pri>o->pri; }
};

//! Push an element @[val] into the priority queue and assign a priority value
//! @[pri] to it. The priority queue will automatically sort itself so that
//! the element with the smallest priority will be at the top.
elem push(int|float pri, mixed val)
{
  elem handle;

  handle=elem(pri, val);
  ::push(handle);
  return handle;
}

//! Adjust the priority value @[new_pri] of an element @[handle] in the
//! priority queue. The priority queue will automatically sort itself so
//! that the element with the smallest priority value will be at the top.
void adjust_pri(mixed handle, int|float new_pri)
{
  handle->set_pri(new_pri);
}

//! Removes and returns the item on top of the heap,
//! which also is the smallest value in the heap.
mixed pop() { return ::pop(); }

//! Returns the item on top of the priority queue (which is also the element
//! with the smallest priority value) without removing it.
mixed peek() { return ::peek(); }
