#pike __REAL_VERSION__

  constant BSONTimestamp = 1;

  protected int timestamp;
  
  //!
  protected void create(int|void _timestamp)
  {
    timestamp = _timestamp;
  }
  
  int get_timestamp()
  {
    return timestamp;
  }
  
  int set_timestamp(int _timestamp)
  {
    timestamp = _timestamp;
  }

