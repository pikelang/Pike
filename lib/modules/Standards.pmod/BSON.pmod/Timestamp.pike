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
    return counter<<32 | timestamp;
  }

  int set_timestamp(int timestamp)
  {
    this::timestamp = timestamp & 0xffffffff;
    counter = timestamp>>32;
  }
