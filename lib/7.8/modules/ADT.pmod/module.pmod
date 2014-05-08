#pike 7.9

class struct
{
  inherit ADT.struct;

  protected void create(void|string s)
  {
    ::create([void|string(8bit)] s);
  }

  void put_var_uint_array(array(int) data, int(0..) item_size, int(0..) len)
  {
    put_uint(sizeof(data), len);
    put_fix_uint_array(data, item_size);
  }

  array(int) get_var_uint_array(int item_size, int len)
  {
    return get_fix_uint_array(item_size, get_uint(len));
  }
}
