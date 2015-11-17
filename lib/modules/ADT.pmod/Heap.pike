#pike __REAL_VERSION__

//! This class implements a (min-)heap. The value of a child node will
//! always be greater than or equal to the value of its parent node.
//! Thus, the top node of the heap will always hold the smallest value.

//! Heap element.
class Element (mixed value)
{
  int pos = -1;

  constant is_adt_heap_element = 1;

  protected int `<(mixed other) { return value < other; }
  protected int `>(mixed other) { return value > other; }
}

#define SWAP(X,Y) do{ mixed tmp=values[X]; (values[X]=values[Y])->pos = X; (values[Y]=tmp)->pos = Y; }while(0)

protected array(Element) values=allocate(10);
protected int num_values;

#ifdef ADT_HEAP_DEBUG
void verify_heap()
{
  for(int e=0; e<num_values; e++) {
    if (!values[e] || !values[e]->is_adt_heap_element)
      error("Error in HEAP: Position %d has no element.\n", e);
    if (values[e]->pos != e)
      error("Error in HEAP: Element %d has invalid position: %d.\n",
	    e, values[e]->pos);
    if(e && (values[(e-1)/2] > values[e]))
      error("Error in HEAP (%d, %d) num_values=%d\n",
	    (e-1)/2, e, num_values);
  }
}
#else
#define verify_heap()
#endif

protected void adjust_down(int elem)
{
  while(1)
  {
    int child=elem*2+1;
    if(child >= num_values) break;

    if(child+1==num_values || values[child] < values[child+1])
    {
      if(values[child] < values[elem])
      {
	SWAP(child, elem);
	elem=child;
	continue;
      }
    } else {
      if(child+1 >= num_values) break;

      if(values[child+1] < values[elem])
      {
	SWAP(elem, child+1);
	elem=child+1;
	continue;
      }
    }
    break;
  }
}

protected int adjust_up(int elem)
{
  int parent=(elem-1)/2;

  if(elem && values[elem] < values[parent])
  {
    SWAP(elem, parent);
    elem=parent;
    while(elem && (values[elem] < values[parent=(elem -1)/2]))
    {
      SWAP(elem, parent);
      elem=parent;
    }
    adjust_down(elem);
    return 1;
  }
  return 0;
}

//! Push an element onto the heap. The heap will automatically sort itself
//! so that the smallest value will be at the top.
//!
//! @returns
//!   Returns an element handle, which can be used with
//!   @[adjust()] and @[remove()].
//!
//! @seealso
//!   @[pop()], @[remove()]
Element push(mixed value)
{
  Element ret;
  if (objectp(value) && value->is_adt_heap_element) {
    ret = value;
  } else {
    ret = Element(value);
  }
  if(num_values >= sizeof(values))
    values+=allocate(10+sizeof(values)/4);

  (values[num_values] = ret)->pos = num_values++;
  adjust_up(num_values-1);
  verify_heap();
  return ret;
}

//! Takes a value in the heap and sorts it through the heap to maintain
//! its sort criteria (increasing order).
//!
//! @param value
//!   Either the element handle returned by @[push()], or the pushed
//!   value itself.
//!
//! @returns
//!   Returns the element handle for the value (if present in the heap),
//!   and @expr{0@} (zero) otherwise.
Element adjust(mixed value)
{
  int pos;
  if (objectp(value) && value->is_adt_heap_element) {
    pos = value->pos;
  } else {
    pos = search(map(values, lambda(Element x) { return x?->value; }), value);
  }
  Element ret;
  if(pos>=0) {
    ret = values[pos];
    if(!adjust_up(pos))
      adjust_down(pos);
  }
  verify_heap();
  return ret;
}

//! Removes and returns the item on top of the heap,
//! which also is the smallest value in the heap.
//!
//! @throws
//!   Throws an error if the heap is empty.
//!
//! @seealso
//!   @[peek()], @[push()], @[remove()]
mixed pop()
{
  if(!num_values)
    error("Heap underflow!\n");

  Element value = values[0];
  value->pos = -1;
  num_values--;
  if(num_values)
  {
    (values[0] = values[num_values])->pos = 0;
    adjust_down(0);

    if(num_values * 3 + 10 < sizeof(values))
      values=values[..num_values+10];
  }
  values[num_values]=0;
  verify_heap();
  return value->value;
}

//! Returns the number of elements in the heap.
int _sizeof() { return num_values; }

//! Returns the item on top of the heap (which is also the smallest value
//! in the heap) without removing it.
//!
//! @returns
//!   Returns the smallest value on the heap if any, and
//!   @expr{UNDEFINED@} otherwise.
//!
//! @seealso
//!   @[pop()]
mixed peek()
{
  if (!num_values)
    return UNDEFINED;

  return values[0]->value;
}

//! Remove a value from the heap.
//!
//! @param value
//!   Value to remove.
//!
//! @seealso
//!   @[push()], @[pop()]
void remove(mixed value)
{
  int pos;
  if (objectp(value) && value->is_adt_heap_element) {
    pos = value->pos;
  } else {
    pos = search(map(values, lambda(Element x) { return x?->value; }), value);
  }
  if ((pos < 0) || (pos >= num_values)) return;

  value = values[pos];
  values[pos] = values[--num_values];
  values[num_values] = 0;
  value->pos = -1;
  if (pos < num_values) {
    if (!adjust_up(pos))
      adjust_down(pos);
  }

  if(num_values * 3 + 10 < sizeof(values))
    values=values[..num_values+10];

  verify_heap();
}
