inherit .Heap;

class elem {
  int pri;
  mixed value;
  
  void create(int a, mixed b) { pri=a; value=b; }

  void set_pri(int p)
    {
      pri=p;
      adjust(this_object());
    }

  int get_pri() { return pri; }
  
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

mixed pop() { return top()->value; }
