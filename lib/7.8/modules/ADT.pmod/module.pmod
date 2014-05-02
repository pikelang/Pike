#pike 7.9

class struct
{
  inherit ADT.struct;

  protected void create(void|string s)
  {
    ::create([void|string(8bit)] s);
  }
}
