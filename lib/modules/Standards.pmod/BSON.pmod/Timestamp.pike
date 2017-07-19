#pike __REAL_VERSION__

  constant BSONTimestamp = 1;

  int timestamp;
  int counter;

  //!
  protected void create()
  {
  }

  variant protected void create(int counter, int timestamp)
  {
    this::counter = counter;
    this::timestamp = timestamp;
  }

  int get_timestamp()
  {
    return timestamp<<32 | counter;
  }

  int set_timestamp(int ticks)
  {
    counter = ticks & 0xffffffff;
    this::timestamp = ticks >> 32;
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%O, %O)", this_program, counter, timestamp);
  }

  protected int(0..1) _equal(mixed o)
  {
    return objectp(o) && o->counter==counter && o->timestamp==timestamp;
  }
