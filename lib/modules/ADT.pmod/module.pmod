#pike __REAL_VERSION__
#pragma strict_types
#if constant(_ADT)
inherit _ADT;
#endif /* _ADT */

//! String buffer with the possibility to read and write data
//! as they would be formatted in structs.
class struct {

  string buffer;
  int index;

  //! Create a new buffer, optionally initialized with the
  //! value @[s].
  void create(void|string s)
  {
    buffer = s || "";
    index = 0;
  }

  //! Trims the buffer to only contain the data after the
  //! read pointer and returns the contents of the buffer.
  string contents()
  {
    buffer = buffer[index..];
    index = 0;
    return buffer;
  }

  //! Adds the data @[s] verbatim to the end of the buffer.
  void add_data(string s)
  {
    buffer += s;
  }

  //! Return all the data in the buffer and empties it.
  string pop_data()
  {
    string res = buffer;
    create();
    return res;
  }

  //! Appends an unsigned integer in network order to the buffer.
  //!
  //!  @param i
  //!    Unsigned integer to append.
  //!  @param len
  //!    Length of integer in bytes.
  void put_uint(int i, int len)
  {
    if (i<0)
      error("Negative argument.\n");
    add_data(sprintf("%*c", len, i));
  }

  //! Appends a variable string @[s] preceded with an unsigned integer
  //! of the size @[len_width] declaring the length of the string. The
  //! string @[s] should be 8 bits wide.
  void put_var_string(string s, int len_width)
  {
    if ( (len_width <= 3) &&
	 (sizeof(s) >= ({ -1, 0x100, 0x10000, 0x1000000 })[len_width] ))
      error("Field overflow.\n");
    put_uint(sizeof(s), len_width);
    add_data(s);
  }

#if constant(Gmp.mpz)
  //! Appends a bignum @[i] as a variable string preceded with an
  //! unsigned integer of the size @[len_width] declaring the length
  //! of the string. @[len_width] defaults to 2.
  void put_bignum(Gmp.mpz i, int|void len_width)
  {
    if (i<0)
      error("Negative argument.\n");
    put_var_string(i->digits(256), len_width || 2);
  }
#endif

  //! Appends the fix sized string @[s] to the buffer.
  void put_fix_string(string s)
  {
    add_data(s);
  }

  //! Appends an array of unsigned integers of width @[item_size]
  //! to the buffer.
  void put_fix_uint_array(array(int) data, int item_size)
  {
    foreach(data, int i)
      put_uint(i, item_size);
  }

  //! Appends an array of unsigned integers of width @[item_size]
  //! to the buffer, preceded with an unsigned integer @[len] declaring
  //! the size of the array.
  void put_var_uint_array(array(int) data, int item_size, int len)
  {
    put_uint(sizeof(data), len);
    put_fix_uint_array(data, item_size);
  }

  //! Reads an unsigned integer from the buffer.
  int(0..) get_uint(int len)
  {
    int(0..) i;
    if ( (sizeof(buffer) - index) < len)
      error("No data.\n");
    sscanf(buffer, "%*" + (string) index +"s%" + (string) len + "c", i);
    index += len;
    return i;
  }

  //! Reads a fixed sized string of length @[len] from the buffer.
  string get_fix_string(int len)
  {
    if ((sizeof(buffer) - index) < len)
      error("No data\n");

    string res = buffer[index .. index + len - 1];
    index += len;
    return res;
  }

  //! Reads a string written by @[put_var_string] from the buffer.
  string get_var_string(int len)
  {
    return get_fix_string(get_uint(len));
  }

#if constant(Gmp.mpz)
  //! Reads a bignum written by @[put_bignum] from the buffer.
  Gmp.mpz get_bignum(int|void len)
  {
    return Gmp.mpz(get_var_string(len || 2), 256);
  }
#endif

  //! Get the remaining data from the buffer and clears the buffer.
  string get_rest()
  {
    string s = buffer[index..];
    create();
    return s;
  }

  //! Reads an array of integers as written by @[put_fix_uint_array]
  //! from the buffer.
  array(mixed) get_fix_uint_array(int item_size, int size)
  {
    array(mixed) res = allocate(size);
    for(int i = 0; i<size; i++)
      res[i] = get_uint(item_size);
    return res;
  }

  //! Reads an array of integers as written by @[put_var_uint_array]
  //! from the buffer.
  array(mixed) get_var_uint_array(int item_size, int len)
  {
    return get_fix_uint_array(item_size, get_uint(len));
  }

  //! Returns one of there is any more data to read.
  int(0..1) is_empty()
  {
    return (index == sizeof(buffer));
  }
}
