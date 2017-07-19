#pike __REAL_VERSION__

  constant BSONBinary = 1;

  string data;
  int subtype = 0x00;

  // NB: Code duplication from module.pmod to avoid circular dependencies.
  private constant BINARY_OLD = 0x02;

  //!
  protected void create(string data, int|void subtype)
  {
    this::subtype = subtype;
    this::data = data;
    if(subtype == BINARY_OLD)
    {
      if( !sscanf(data, "%-4H", data) )
        throw(Error.Generic("old binary data length does not match actual data length.\n"));
    }
  }

  int get_subtype()
  {
    return subtype;
  }

  void set_subtype(int _subtype)
  {
    subtype = _subtype;
  }

  protected int _sizeof()
  {
    if(subtype == BINARY_OLD)
      return sizeof(data) + 4;
    else
      return sizeof(data);
  }

  protected mixed cast(string type)
  {
    if(type == "string")
      return data;
    return UNDEFINED;
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(0x%02x, %O)", this_program, subtype, data);
  }
