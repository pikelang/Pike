#pike __REAL_VERSION__

  constant BSONBinary = 1;

  protected string data;
  protected int subtype = 0x00;

  // NB: Code duplication from module.pmod to avoid circular dependencies.
  private constant BINARY_OLD = 0x02;

  //!
  protected void create(string _data, int|void _subtype)
  {
     subtype = _subtype;
     if(subtype == BINARY_OLD)
     {
       if( !sscanf(_data, "%-4H", data) )
         throw(Error.Generic("old binary data length does not match actual data length.\n"));
     }
     else
       data = _data;
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
