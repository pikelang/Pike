#pike __REAL_VERSION__
#pragma strict_types

//! Various Abstract Data Types.

#if constant(_ADT)
inherit _ADT;
#endif /* _ADT */

constant List = __builtin.List;

// Internal stuff for ADT.Struct
protected int item_counter;
int get_item_id() { return item_counter++; }

protected class structError
{
  inherit Error.Generic;
  constant ADT_struct = 1;
}

//! String buffer with the possibility to read and write data
//! as they would be formatted in structs.
class struct {

  string(0..255) buffer;
  int index;

  //! Create a new buffer, optionally initialized with the
  //! value @[s].
  void create(void|string(0..255) s)
  {
    buffer = s || "";
    index = 0;
  }

  //! Trims the buffer to only contain the data after the
  //! read pointer and returns the contents of the buffer.
  string(0..255) contents()
  {
    buffer = buffer[index..];
    index = 0;
    return buffer;
  }

  //! Adds the data @[s] verbatim to the end of the buffer.
  this_program add_data(string(0..255) s)
  {
    buffer += s;
    return this;
  }

  //! Return all the data in the buffer and empties it.
  string(0..255) pop_data()
  {
    string(0..255) res = buffer;
    create();
    return res;
  }

  //! Appends an unsigned integer in network order to the buffer.
  //!
  //!  @param i
  //!    Unsigned integer to append.
  //!  @param len
  //!    Length of integer in bytes.
  this_program put_uint(int i, int(0..) len)
  {
    if (i<0)
      throw(structError("Negative argument.\n"));
    add_data([string(0..255)]sprintf("%*c", len, i));
    return this;
  }

  //! Appends a variable string @[s] preceded with an unsigned integer
  //! of the size @[len_width] declaring the length of the string. The
  //! string @[s] should be 8 bits wide.
  this_program put_var_string(string(0..255) s, int(0..) len_width)
  {
    add_data([string(0..255)]sprintf("%*H", len_width, s));
    return this;
  }

  //! Appends a bignum @[i] as a variable string preceded with an
  //! unsigned integer of the size @[len_width] declaring the length
  //! of the string. @[len_width] defaults to 2.
  this_program put_bignum(Gmp.mpz i, int(0..)|void len_width)
  {
    if (i<0)
      throw(structError("Negative argument.\n"));
    put_var_string(i->digits(256), len_width || 2);
    return this;
  }

  //! Appends the fix sized string @[s] to the buffer.
  this_program put_fix_string(string(0..255) s)
  {
    add_data(s);
    return this;
  }

  //! Appends an array of unsigned integers of width @[item_size]
  //! to the buffer.
  this_program put_fix_uint_array(array(int) data, int(0..) item_size)
  {
    foreach(data, int i)
      put_uint(i, item_size);
    return this;
  }

  //! Appends an array of unsigned integers of width @[item_size]
  //! to the buffer, preceded with an unsigned integer @[len] declaring
  //! the size of the array.
  this_program put_var_uint_array(array(int) data, int(0..) item_size, int(0..) len)
  {
    put_uint(sizeof(data), len);
    put_fix_uint_array(data, item_size);
    return this;
  }

  //! Reads an unsigned integer from the buffer.
  int(0..) get_uint(int len)
  {
    int(0..) i;
    if ( (sizeof(buffer) - index) < len)
      throw(structError("No data.\n"));
    sscanf(buffer, "%*" + (string) index +"s%" + (string) len + "c", i);
    index += len;
    return i;
  }

  //! Reads a fixed sized string of length @[len] from the buffer.
  string(0..255) get_fix_string(int len)
  {
    if ((sizeof(buffer) - index) < len)
      throw(structError("No data\n"));

    string(0..255) res = buffer[index .. index + len - 1];
    index += len;
    return res;
  }

  //! Reads a string written by @[put_var_string] from the buffer.
  string(0..255) get_var_string(int len)
  {
    return get_fix_string(get_uint(len));
  }

  //! Reads a bignum written by @[put_bignum] from the buffer.
  Gmp.mpz get_bignum(int|void len)
  {
    return Gmp.mpz(get_var_string(len || 2), 256);
  }

  //! Get the remaining data from the buffer and clears the buffer.
  string(0..255) get_rest()
  {
    string(0..255) s = buffer[index..];
    create();
    return s;
  }

  //! Reads an array of integers as written by @[put_fix_uint_array]
  //! from the buffer.
  array(int) get_fix_uint_array(int item_size, int size)
  {
    array(int) res = allocate(size);
    for(int i = 0; i<size; i++)
      res[i] = get_uint(item_size);
    return res;
  }

  //! Reads an array of integers as written by @[put_var_uint_array]
  //! from the buffer.
  array(int) get_var_uint_array(int item_size, int len)
  {
    return get_fix_uint_array(item_size, get_uint(len));
  }

  //! Returns one of there is any more data to read.
  int(0..1) is_empty()
  {
    return (index == sizeof(buffer));
  }

  protected int(0..) _sizeof()
  {
    return [int(0..)](sizeof(buffer)-index);
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%O)", this_program, _sizeof());
  }
}
