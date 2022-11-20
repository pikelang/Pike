#pike __REAL_VERSION__

inherit .Priority_queue;

//! This class implements a resource scheduler. It is a tree where each
//! node has a weight which indicates its share of the resource among
//! its siblings, and a parent-child relation, where a child only gets
//! resources if the parent is inactive.
//!
//! Active nodes are kept in a @[Priority_queue] sorted on a dynamic
//! priority value that depends on the amount of resources allocated
//! to the node so far.

class elem {
  inherit ::this_program;

  int(1..) weight;
  elem parent;
  float pri;

  void create(int(1..) w, mixed v, elem|void parent) {
    weight = w;
    value = v;
    this_program::parent = parent || root;
  }

  void set_pri(int p)
  {
    weight = p;
    adjust(this);
  }

  int get_pri() { return weight; }

};

elem root = elem(0, 0);

//! Push an element @[val] into the priority queue and assign a priority value
//! @[pri] to it. The priority queue will automatically sort itself so that
//! the element with the smallest priority will be at the top.
mixed push(int pri, mixed val)
{
  mixed handle;

  handle=elem(pri, val);
  ::push(handle);
  return handle;
}

//! Adjust the priority value @[new_pri] of an element @[handle] in the
//! priority queue. The priority queue will automatically sort itself so
//! that the element with the smallest priority value will be at the top.
void adjust_pri(mixed handle, int new_pri)
{
  handle->set_pri(new_pri);
}

//! Removes and returns the item on top of the heap,
//! which also is the smallest value in the heap.
mixed pop() { return ::pop()->value; }

//! Returns the item on top of the priority queue (which is also the element
//! with the smallest priority value) without removing it.
mixed peek()
{
    mixed res = ::peek();
    if ( undefinedp(res) ) return UNDEFINED;
    else return res->value;
}
