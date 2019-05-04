#pike __REAL_VERSION__

//! This class implements a simple stack. Instead of adding and removing
//! elements to an array, and thus making it vary in size for every push
//! and pop operation, this stack tries to keep the stack size constant.
//! If however the stack risks to overflow, it will allocate double its
//! current size, i.e. pushing an element on an full 32 slot stack will
//! result in a 64 slot stack with 33 elements.

// NB: There's apparently quite a bit of code that accesses these
//     directly, so they can't be made protected directly.
//     Any accesses to ptr should be replaced with sizeof(),
//     and any to arr with values().
//
//     The pragma is to be removed when ptr and arr are declared protected.
#pragma no_deprecation_warnings

__deprecated__(int) ptr;
__deprecated__(array) arr;

//! Push an element on the top of the stack.
void push(mixed val)
{
  if(ptr == sizeof(arr)) {
    arr += allocate(ptr);
  }
  arr[ptr++] = val;
}

//! Returns the top element from the stack, without
//! popping it.
//! @throws
//!   Throws an error if called on an empty stack.
//! @seealso
//!   @[peek()]
mixed top()
{
  if (ptr) {
    return arr[ptr-1];
  }
  error("Stack underflow\n");
}

//! Returns an element from the stack, without popping it.
//!
//! @param offset
//!   The number of elements from the top of the stack to skip.
//!
//! @throws
//!   Throws an error if called on an empty stack.
//!
//! @seealso
//!   @[top()]
mixed peek(int|void offset)
{
  if ((offset >= 0) && ((ptr-offset) > 0)) {
    return arr[ptr-offset-1];
  }
  error("Stack underflow\n");
}

//! Pops @[val] entries from the stack, or one entry
//! if no value is given. The popped entries are not
//! actually freed, only the stack pointer is moved.
void quick_pop(void|int val)
{
  if (val) {
    if (ptr < val) {
	ptr = 0;
    } else {
	ptr -= val;
    }
  } else {
    if (ptr > 0) {
	ptr--;
    }
  }
}

//! Pops and returns entry @[val] from the stack, counting
//! from the top. If no value is given the top element is
//! popped and returned. All popped entries are freed from
//! the stack.
mixed pop(void|int val)
{
  mixed ret;

  if (val) {
    if (ptr <= 0) {
	error("Stack underflow\n");
    }

    if (ptr < val) {
      val = ptr;
    }
    ptr -= val;
    ret = arr[ptr..ptr + val - 1];

    for (int i=0; i < val; i++) {
      arr[ptr + i] = 0;       /* Don't waste references */
    }
  } else {
    if(--ptr < 0)
	error("Stack underflow\n");

    ret = arr[ptr];
    arr[ptr]=0; /* Don't waste references */
  }
  return ret;
}

//! Empties the stack, resets the stack pointer
//! and shrinks the stack size to the given value
//! or 32 if none is given.
//! @seealso
//!   @[create]
void reset(int|void initial_size)
{
  arr = allocate(initial_size || 32);
  ptr = 0;
}

//! An initial stack size can be given when
//! a stack is cloned. The default value is
//! 32.
protected void create(int|void initial_size)
{
  arr = allocate(initial_size || 32);
}

//! Sets the stacks content to the provided array.
void set_stack(array stack) {
  arr = stack;
  ptr = sizeof(arr);
}

//! @[sizeof] on a stack returns the number of entries
//! in the stack.
protected int _sizeof() {
  return ptr;
}

//! @[values] on a stack returns all the entries in
//! the stack, in order.
protected array _values() {
  return arr[..ptr-1];
}

//! Return the stack-depth to @[item].
//!
//! This function makes it possible to use
//! eg @[search()] and @[has_value()] on the stack.
protected int _search(mixed item)
{
  int i;
  for (i = ptr; i--;) {
    if (arr[i] == item) return ptr-(i+1);
  }
  return -1;
}

//! A stack added with another stack yields a new
//! stack with all the elements from both stacks,
//! and the elements from the second stack at the
//! top of the new stack.
protected this_program `+(this_program s) {
  array elem = arr[..ptr-1]+values(s);
  this_program ns = this_program(1);
  ns->set_stack(elem);
  return ns;
}

protected mixed cast(string to)
{
  if( to=="array" )
    return _values();
  return UNDEFINED;
}

protected string _sprintf(int t) {
  return t=='O' && sprintf("%O%O", this_program, _values());
}
