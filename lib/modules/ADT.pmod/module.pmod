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
  inherit Stdio.Buffer;

  //! Create a new buffer, optionally initialized with the
  //! value @[s].
  void create(void|string(0..255) s)
  {
     if( s && strlen(s) )
         ::create(s);
     else
         ::create();
     set_error_mode(structError);
  }

  //! Trims the buffer to only contain the data after the
  //! read pointer and returns the contents of the buffer.
  string(0..255) contents()
  {
      return (string(8bit))this;
  }

  //! Adds the data @[s] verbatim to the end of the buffer.
  this_program add_data(string(0..255) s)
  {
      return [object(this_program)]add(s);
  }

  //! Return all the data in the buffer and empties it.
  string(0..255) pop_data()
  {
    return read();
  }

  //! Appends an unsigned integer in network order to the buffer.
  //!
  //!  @param i
  //!    Unsigned integer to append.
  //!  @param len
  //!    Length of integer in bytes.
  this_program put_uint(int i, int(0..) len)
  {
    return [object(this_program)]add_int(i,len);
  }

  //! Appends a variable string @[s] preceded with an unsigned integer
  //! of the size @[len_width] declaring the length of the string. The
  //! string @[s] should be 8 bits wide.
  this_program put_var_string(string(0..255) s, int(0..) len_width)
  {
    return [object(this_program)]add_hstring(s,len_width);
  }

  //! Appends a bignum @[i] as a variable string preceded with an
  //! unsigned integer of the size @[len_width] declaring the length
  //! of the string. @[len_width] defaults to 2.
  this_program put_bignum(Gmp.mpz i, int(0..)|void len_width)
  {
    return [object(this_program)]add_hstring(i->digits(256),len_width||2);
  }

  //! Appends the fix sized string @[s] to the buffer.
  this_program put_fix_string(string(0..255) s)
  {
    return [object(this_program)]add(s);
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

  //! Reads an unsigned integer from the buffer.
  int(0..) get_uint(int len)
  {
    return read_int(len);
  }

  //! Reads a fixed sized string of length @[len] from the buffer.
  string(0..255) get_fix_string(int len)
  {
     return read(len);
  }

  //! Reads a string written by @[put_var_string] from the buffer.
  string(0..255) get_var_string(int len)
  {
     return read_hstring(len);
  }

  //! Reads a bignum written by @[put_bignum] from the buffer.
  Gmp.mpz get_bignum(int|void len)
  {
      return Gmp.mpz(read_hstring(len||2),256);
  }

  //! Get the remaining data from the buffer and clears the buffer.
  string(0..255) get_rest()
  {
    return read();
  }

  //! Reads an array of integers as written by @[put_fix_uint_array]
  //! from the buffer.
  array(int) get_fix_uint_array(int item_size, int size)
  {
    return read_ints(size, item_size);
  }

  //! Reads an array of integers as written by @[put_var_uint_array]
  //! from the buffer.
  array(int) get_var_uint_array(int item_size, int len)
  {
    int size = read_int(len);
    int elems = size/item_size;
    if( elems*item_size != size )
      throw(structError("Impossible uint array length value.\n"));
    return read_ints(elems, item_size);
  }

  //! Returns one if there is any more data to read.
  int(0..1) is_empty()
  {
    return !sizeof(this);
  }
}
