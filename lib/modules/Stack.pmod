#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

class stack {
  int ptr;
  mixed *arr=allocate(32);

  void push(mixed val)
  {
    if(ptr==sizeof(arr)) arr+=allocate(ptr);
    arr[ptr++]=val;
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

  void reset()
  {
    arr=allocate(32);
    ptr=0;
  }
};

