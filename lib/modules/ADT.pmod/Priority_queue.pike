#pike __REAL_VERSION__

//! Type for the individual elements in the queue.
__generic__ ValueType;

inherit .Heap(<ValueType>);

//! This class implements a priority queue. Each element in the priority
//! queue is assigned a priority value, and the priority queue always
//! remains sorted in increasing order of the priority values. The top of
//! the priority queue always holds the element with the smallest priority.
//! The priority queue is realized as a (min-)heap.

//! Priority queue element.
class Element (<ValueType = ValueType>) {
  inherit ::this_program(<ValueType>);

  //! Priority.
  //!
  //! Elements with lower priority are returned by @[pop()] and @[peek()]
  //! before elements with higher.
  //!
  //! @note
  //!   Do NOT alter directly! Use @[set_pri()] or @[adjust_pri()].
  //!
  //! @seealso
  //!   @[set_pri()], @[adjust_pri()], @[pop()], @[peek()]
  int|float pri;

  protected void create(int|float a, ValueType b) { pri=a; ::create(b); }

  //! Set the priority for the element to @[p] and reorder
  //! the queue as appropriate.
  //!
  //! @seealso
  //!   @[adjust_pri()]
  void set_pri(int|float p)
    {
      pri=p;
      adjust(this);
    }

  //! Return the current priority.
  int|float get_pri() { return pri; }

  protected int `<(object o) { return pri<o->pri; }
  protected int `>(object o) { return pri>o->pri; }
};

//! Push an element @[val] into the priority queue and assign a priority value
//! @[pri] to it. The priority queue will automatically sort itself so that
//! the element with the smallest priority will be at the top.
Element push(int|float pri, ValueType val)
{
  Element handle;

  handle = Element(pri, val);
  ::push(handle);
  return handle;
}

//! Adjust the priority value @[new_pri] of an element @[handle] in the
//! priority queue. The priority queue will automatically sort itself so
//! that the element with the smallest priority value will be at the top.
void adjust_pri(Element handle, int|float new_pri)
{
  handle->set_pri(new_pri);
}

//! Removes and returns the item on top of the heap,
//! which also is the smallest value in the heap.
//!
//! @seealso
//!   @[peek()]
ValueType pop() { return ::pop(); }

//! Returns the item on top of the priority queue (which is also the element
//! with the smallest priority value) without removing it.
//!
//! @seealso
//!   @[pop()]
ValueType peek() { return ::peek(); }
