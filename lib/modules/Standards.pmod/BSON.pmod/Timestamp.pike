#pike __REAL_VERSION__

  constant BSONTimestamp = 1;

  int timestamp;
  int(0..) counter;

  //!
  protected void create()
  {
  }

  variant protected void create(int(0..) counter, int timestamp)
  {
    if (counter < 0) {
      error("Invalid counter.\n");
    }
    this::counter = counter;
    if (timestamp > 0x7fffffff) {
      error("Invalid timestamp.\n");
    }
    this::timestamp = timestamp;
  }

  variant protected void create(int ticks)
  {
    this::counter = ticks & 0xffffffff;
    this::timestamp = ticks >> 32;
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
