#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

class stack {
  int ptr;
  mixed *arr=allocate(32);

  void push(mixed val)
  {
    if(ptr==sizeof(arr)) arr+=allocate(ptr);
    arr[ptr++]=val;
  }

  mixed pop(mixed val)
  {
    mixed foo;
    if(--ptr < 0)
      error("Stack underflow\n");
    
    foo=arr[ptr];
    arr[ptr]=0; /* Don't waste references */
    return foo;
  }

  void reset()
  {
    arr=allocate(32);
    ptr=0;
  }
};

void create()
{
  master()->add_precompiled_program("/precompiled/stack",stack);
}
