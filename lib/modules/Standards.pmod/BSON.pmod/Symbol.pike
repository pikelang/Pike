#pike __REAL_VERSION__

  constant BSONSymbol = 1;

  static string data;
  
  //!
  static void create(string _data)
  {
     data = _data;
  }
  
  static int _sizeof()
  { 
    return sizeof(data);
  }
  
  static mixed cast(string type)
  {
    if(type == "string")
      return data;
  }

