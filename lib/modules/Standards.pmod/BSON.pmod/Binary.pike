  constant BSONBinary = 1;

  static string data;
  static int subtype = 0x00;
  
  //!
  static void create(string _data, int|void _subtype)
  {
     subtype = _subtype;
     int len;
     if(subtype == .BINARY_OLD)
     {
       sscanf(data, "%-4c%s", len, data);
       if(sizeof(data) != len) throw(Error.Generic("old binary data length does not match actual data length.\n"));
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
  static int _sizeof()
  { 
    if(subtype == .BINARY_OLD)
      return sizeof(data) + 4;
    else
      return sizeof(data);
  }
  
  static mixed cast(string type)
  {
    if(type == "string")
    {
    	    // the docs are a little sketchy about this, do we need to NULL terminate?
       if(subtype == .BINARY_OLD)
         return sprintf("%-4c%s", sizeof(data), data);
       else return data;
    }
  }

