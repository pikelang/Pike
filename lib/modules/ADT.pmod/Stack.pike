
//! This class implements a simple stack. Instead of adding and removing
//! elements to an array, and thus making it vary in size for every push
//! and pop operation, this stack tries to keep the stack size constant.
//! If however the stack risks to overflow, it will allocate double its
//! current size, i.e. pushing an element on an full 32 slot stack will
//! result in a 64 slot stack with 33 elements.

int ptr;
array arr;

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
mixed top()
{
  if (ptr) {
    return(arr[ptr-1]);
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
//! popped and returned. All poped entries are freed from
//! the stack.
mixed pop(void|int val)
{
  mixed foo;

  if (val) {
    if (ptr <= 0) {
	error("Stack underflow\n");
    }

    if (ptr < val) {
      val = ptr;
    }
    ptr -= val;
    foo = arr[ptr..ptr + val - 1];

    for (int i=0; i < val; i++) {
      arr[ptr + i] = 0;       /* Don't waste references */
    }
  } else {
    if(--ptr < 0)
	error("Stack underflow\n");
  
    foo=arr[ptr];
    arr[ptr]=0; /* Don't waste references */
  }
  return foo;
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
void create(int|void initial_size)
{
  arr = allocate(initial_size || 32);
}
