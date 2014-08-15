#pike __REAL_VERSION__

  constant BSONSymbol = 1;

  protected string data;
  
  //!
  protected void create(string _data)
  {
     data = _data;
  }
  
  protected int _sizeof()
  { 
    return sizeof(data);
  }
  
  protected mixed cast(string type)
  {
    if(type == "string")
      return data;
  }

