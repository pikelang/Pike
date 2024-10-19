
//! A simple FIFO queue.

#pike __REAL_VERSION__
#pragma strict_types

//! Type for the individual elements in the queue.
__generic__ ValueType;

array(ValueType)|zero l;

//! Creates a queue with the initial items @[args] in it.
protected void create(ValueType ...args)
{
  l = args;
}

protected int _sizeof()
{
  return sizeof(l);
}

protected array(ValueType) _values()
{
  return values(l);
}

//! @deprecated put
__deprecated__ void write(ValueType ... items)
{
  l += items;
}

//! @decl void put(ValueType ... items)
//! Adds @[items] to the queue.
//!
//! @seealso
//!   @[get()], @[peek()]
void put(ValueType ... items)
{
  l += items;
}

//! @deprecated get
__deprecated__ ValueType read()
{
  return get();
}

//! @decl ValueType get()
//! Returns the next element from the queue, or @expr{UNDEFINED@} if
//! the queue is empty.
//!
//! @seealso
//!   @[peek()], @[put()]
object(ValueType)|zero get()
{
  if( !sizeof(l) ) return UNDEFINED;
  ValueType res = l[0];
  l = l[1..];
  return res;
}

//! Returns the next element from the queue without removing it from
//! the queue. Returns @expr{UNDEFINED@} if the queue is empty.
//!
//! @note
//!   Prior to Pike 9.0 this function returned a plain @expr{0@}
//!   when the queue was empty.
//!
//! @seealso
//!   @[get()], @[put()]
object(ValueType)|zero peek()
{
  return sizeof(l)?l[0]:UNDEFINED;
}

//! Returns true if the queue is empty, otherwise zero.
int(0..1) is_empty()
{
  return !sizeof(l);
}

//! Empties the queue.
void flush()
{
  l = ({});
}

//! It is possible to cast ADT.Queue to an array.
protected array(ValueType) cast(string to)
{
  if( to=="array" )
    return l+({});
  return UNDEFINED;
}

protected string _sprintf(int t) {
  return t=='O' && sprintf("%O(%O)", this_program, l);
}
