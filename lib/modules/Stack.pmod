#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

class stack {
  int ptr;
  mixed *arr;

  void push(mixed val)
  {
    if(ptr == sizeof(arr)) {
      arr += allocate(ptr);
    }
    arr[ptr++] = val;
  }

  mixed top()
  {
    if (ptr) {
      return(arr[ptr-1]);
    }
    error("Stack underflow\n");
  }

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

  void reset(int|void initial_size)
  {
    arr = allocate(initial_size || 32);
    ptr = 0;
  }

  void create(int|void initial_size)
  {
    arr = allocate(initial_size || 32);
  }
};

