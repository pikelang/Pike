mapping data=([]);
string file;

int savers;

mixed `[](string key)
{
  return data[key];
}

void save()
{
  Stdio.write_file(file,encode_value(data));
}

mixed `[]=(string key, mixed val)
{
  data[key]=val;
  return class delaysave
  {
    void create() { savers++; }
    void destroy() { if(!--savers) save(); }
  }();
}

void create(string f)
{
  file=f;
  catch {
    data=decode_value(Stdio.read_file(file));
  };
  atexit(save);
}


void destroy()
{
  save();
}
