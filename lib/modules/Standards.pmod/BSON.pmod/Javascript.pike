#pike __REAL_VERSION__

  constant BSONJavascript = 1;

  protected string data;
  mapping scope;

  //!
protected void create(string _data, void|mapping _scope)
  {
     data = _data;
     scope = _scope || ([]);
  }

  protected int _sizeof()
  {
    return sizeof(data);
  }

  protected mixed cast(string type)
  {
    if(type == "string")
      return data;
    return UNDEFINED;
  }
