#pike __REAL_VERSION__

inherit .Heap;

class elem {
  int pri;
  mixed value;

  void create(int a, mixed b) { pri=a; value=b; }

  void set_pri(int p)
    {
      pri=p;
      adjust(this);
    }

  int get_pri() { return pri; }

  int `<(object o) { return pri<o->pri; }
  int `>(object o) { return pri>o->pri; }
  int `==(object o) { return pri==o->pri; }
};

//! @fixme
//!   Document this function
mixed push(int pri, mixed val)
{
  mixed handle;

  handle=elem(pri, val);
  ::push(handle);
  return handle;
}

//! @fixme
//!   Document this function
void adjust_pri(mixed handle, int new_pri)
{
  handle->pri=new_pri;
  ::adjust(handle);
}

//! @fixme
//!   Document this function
mixed pop() { return ::pop()->value; }
