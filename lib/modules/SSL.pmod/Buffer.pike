#pike __REAL_VERSION__
#pragma strict_types

//! String buffer with the possibility to read and write data
//! as they would be formatted in structs.

inherit Stdio.Buffer;

this_program add_hstring(string(8bit)|Stdio.Buffer str, int sz)
{
  return [object(this_program)]::add_hstring(str,sz);
}

this_program add_int(int i, int sz)
{
  return [object(this_program)]::add_int(i,sz);
}

this_program add(string(8bit)|Stdio.Buffer str)
{
  return [object(this_program)]::add(str);
}

//! Create a new buffer, optionally initialized with the
//! value @[s].
void create(void|string(8bit) s)
{
  if( s && strlen(s) )
    ::create(s);
  else
    ::create();
  set_error_mode(.BufferError);
}

//! Appends a bignum @[i] as a variable string preceded with an
//! unsigned integer of the size @[len_width] declaring the length
//! of the string. @[len_width] defaults to 2.
this_program put_bignum(Gmp.mpz i, int(0..)|void len_width)
{
  return [object(this_program)]add_hstring(i->digits(256),len_width||2);
}

//! Appends an array of unsigned integers of width @[item_size]
//! to the buffer.
this_program put_fix_uint_array(array(int) data, int(0..) item_size)
{
  return [object(this_program)]add_ints(data,item_size);
}

//! Appends an array of unsigned integers of width @[item_size]
//! to the buffer, preceded with an unsigned integer @[len] declaring
//! the size of the array in bytes.
this_program put_var_uint_array(array(int) data, int(0..) item_size, int(0..) len)
{
  add_int(sizeof(data)*item_size, len );
  return [object(this_program)]add_ints(data,item_size);
}

//! Appends an array of variable length strings with @[item_size]
//! bytes hollerith coding, prefixed by a @[len] bytes large integer
//! declaring the total size of the array in bytes.
this_program put_var_string_array(array(string(8bit)) data, int(0..) item_size, int(0..) len)
{
  Stdio.Buffer sub = Stdio.Buffer();
  foreach(data, string(8bit) s)
    sub->add_hstring(s, item_size);
  add_int(sizeof(sub),len);
  return [object(this_program)]add(sub);
}

//! Reads a bignum written by @[put_bignum] from the buffer.
Gmp.mpz get_bignum(int|void len)
{
  return Gmp.mpz(read_hstring(len||2),256);
}

//! Reads an array of integers as written by @[put_var_uint_array]
//! from the buffer.
array(int) get_var_uint_array(int item_size, int len)
{
  int size = read_int(len);
  int elems = size/item_size;
  if( elems*item_size != size )
    throw(.BufferError("Impossible uint array length value.\n"));
  return read_ints(elems, item_size);
}
