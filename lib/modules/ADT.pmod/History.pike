#pike __REAL_VERSION__

// $Id: History.pike,v 1.5 2002/10/19 13:42:30 nilsson Exp $

//! A history is a stack where you can only push entries. When the stack has
//! reached a certain size the oldest entries are removed on every push.
//! Other proposed names for this data type is leaking stack and table
//! (where you push objects onto the table in one end and objects are falling
//! off the table in the other.

// The stack where the values are stored.
private array stack;

// A pointer to the top of the stack.
private int top;

// The number of elements currently in the history.
private int size;

// The maximum number of entries in the history at one time.
private int maxsize;

// The sequence number of the latest entry to be pused on
// the history stack.
private int latest_entry_num;

//! @decl void create(int max_size)
//! @[max_size] is the maximum number of entries that can reside in the
//! history at the same time.
void create(int _maxsize) {
  stack = allocate(_maxsize);
  maxsize = _maxsize;
  /*
  int top = 0;
  int size = 0;
  int latest_entry_num = 0;
  */
}

//! Push a new value into the history.
void push(mixed value) {
  if(!maxsize) return;
  stack[top++] = value;
  if(top==maxsize)
    top = 0;
  if(size<maxsize)
    size++;
  latest_entry_num++;
}

//! A @[sizeof] operation on this object returns the number
//! of elements currently in the history, e.g. <= the current
//! max size.
int _sizeof() { return size; }

//! Returns the maximum number of values in the history
int get_maxsize() { return maxsize; }

//! Returns the absolute sequence number of the latest
//! result inserted into the history.
int get_latest_entry_num() {
  return latest_entry_num;
}

//! Returns the absolute sequence number of the
//! oldest result still in the history. Returns 0
//! if there are no results in the history.
int get_first_entry_num() {
  if(!size) return 0;
  return latest_entry_num - size + 1;
}

int remap(int i) {
  return (i+top)%maxsize;
}

//! Get a value from the history as if it was an array, e.g.
//! both positive and negative numbers may be used. The positive
//! numbers are however offset with 1, so [1] is the first entry
//! in the history and [-1] is the last.
mixed `[](int i) {
  if(i<0) {
    if(i<-size)
      error("Only %d entries in history.\n", size);
    return stack[(i+top)%maxsize];
  }
  if(i>latest_entry_num)
    error("Only %d entries in history.\n", size);
  if(i-get_first_entry_num() < 0)
    error("The oldest history entry is %d.\n", get_first_entry_num());

  // ( (i-latest_entry_num+size-1) + (top + (maxsize - size)) )%maxsize
  return stack[(i-latest_entry_num-1+top)%maxsize];
}

//! Set the maximume number of entries that can be
//! stored in the history simultaneous.
void set_maxsize(int _maxsize) {
  if(_maxsize==maxsize)
    return;

  array old_values;
  if(size<maxsize)
    old_values = stack;
  else {
    old_values = stack[top..] + stack[..top-1];
    top = maxsize;
  }

  if(_maxsize>maxsize)
    stack = old_values + allocate(_maxsize-maxsize);
  else {
    stack = old_values[maxsize-_maxsize..];
    size = _maxsize;
    top = 0;
  }
  maxsize = _maxsize;
}

//! Empties the history. All entries in the history are
//! removed, to allow garbage collect to remove them.
//! The entry sequence counter is not reset.
void flush() {
  stack = allocate(maxsize);
  top = 0;
  size = 0;
}

string _sprintf(int t) {
  if(t=='O') return "ADT.History("+size+"/"+maxsize+")";
  if(t=='t') return "ADT.History";
  error("Can't print History object as '%c'.\n", t);
}
