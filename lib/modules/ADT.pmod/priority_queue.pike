inherit "heap";

class elem {
  int pri;
  mixed value;
  
  void create(int a, mixed b) { pri=a; value=b; }
  
  int `<(object o) { return pri<o->pri; }
  int `>(object o) { return pri>o->pri; }
  int `==(object o) { return pri==o->pri; }
};

mixed push(int pri, mixed val)
{
  mixed handle;
  
  handle=elem(pri, val);
  ::push(handle);
  return handle;
}

void adjust_pri(mixed handle, int new_pri)
{
  handle->pri=new_pri;
  ::adjust(handle);
}

mixed next() { return top()->value; }
