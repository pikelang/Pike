#pike __REAL_VERSION__

// $Id$

//! A history is a stack where you can only push entries. When the stack has
//! reached a certain size the oldest entries are removed on every push.
//! Other proposed names for this data type is leaking stack and table
//! (where you push objects onto the table in one end and objects are falling
//! off the table in the other.

// The stack where the values are stored.
private array stack;

// A pointer to the top of the stack.
private int(0..) top;

// The number of elements currently in the history.
private int(0..) size;

// The maximum number of entries in the history at one time.
private int(0..) maxsize;

// The sequence number of the latest entry to be pused on
// the history stack.
private int(0..) latest_entry_num;

// Should we allow identical values to be stored next to each other?
private int(0..1) no_adjacent_duplicates;

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

//! Change how the History object should treat two
//! identical values in a row. If 1 than only unique
//! values are allowed after each other.
//! @seealso
//!   @[query_no_adjacent_duplicates]
void set_no_adjacent_duplicates(int(0..1) i) {
  no_adjacent_duplicates = i;
}

//! Tells if the History object allows adjacent equal
//! values. 1 means that only uniqe values are allowed
//! adter each other.
//! @seealso
//!   @[set_no_adjacent_duplicates]
int(0..1) query_no_adjacent_duplicates() {
  return no_adjacent_duplicates;
}

//! Push a new value into the history.
void push(mixed value) {
  if(!maxsize) return;
  if(size && no_adjacent_duplicates && `[](-1)==value)
    return;
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
//! @seealso
//!   @[set_maxsize]
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

private int(0..) find_pos(int i) {
  if(i<0) {
    if(i<-size)
      error("Only %d entries in history.\n", size);
    return (i+top)%maxsize;
  }
  if(i>latest_entry_num)
    error("Only %d entries in history.\n", size);
  if(i-get_first_entry_num() < 0)
    error("The oldest history entry is %d.\n", get_first_entry_num());

  // ( (i-latest_entry_num+size-1) + (top + (maxsize - size)) )%maxsize
  return (i-latest_entry_num-1+top)%maxsize;
}

//! Get a value from the history as if it was an array, e.g.
//! both positive and negative numbers may be used. The positive
//! numbers are however offset with 1, so [1] is the first entry
//! in the history and [-1] is the last.
mixed `[](int i) {
  return stack[find_pos(i)];
}

//! Overwrite one value in the history. The history position may be
//! identified either by positive or negative offset, like @[`[]].
void `[]=(int i, mixed value) {
  stack[find_pos(i)]=value;
}

//! Set the maximume number of entries that can be
//! stored in the history simultaneous.
//! @seealso
//!   @[get_maxsize]
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

//! Returns the index numbers of the history entries
//! available.
array(int) _indices() {
  return enumerate(size, 1, get_first_entry_num());
}

//! Returns the values of the available history entries.
array _values() {
  return map(_indices(), `[]);
}

string _sprintf(int t) {
  return t=='O' && sprintf("%O(%d/%d)", this_program, size, maxsize);
}
