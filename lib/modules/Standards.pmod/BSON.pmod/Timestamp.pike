#pike __REAL_VERSION__

  constant BSONTimestamp = 1;

  int(0..) timestamp;
  int(0..) counter;

  //!
  protected void create()
  {
  }

  variant protected void create(int(0..) counter, int(0..) timestamp)
  {
    if (counter < 0) {
      error("Invalid counter.\n");
    }
    this::counter = counter;
    if (timestamp < 0 || timestamp > (1<<32)-1 ) {
      error("Invalid timestamp.\n");
    }
    this::timestamp = timestamp;
  }

  variant protected void create(int ticks)
  {
    if (ticks < 0) error("Invalid ticks.\n");
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
