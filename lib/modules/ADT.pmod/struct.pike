/* struct.pike
 *
 * New version, taken from Protocols.X
 */

#pike __REAL_VERSION__

//! generic data structure

string buffer;
int index;

//! create a new struct
//!
//! @param s
//!   initial data in struct
void create(void|string s)
{
  buffer = s || "";
  index = 0;
}

//! Return data, without removing it
//!
//! @returns
//!   contents of struct
string contents()
{
  buffer = buffer[index..];
  index = 0;
  return buffer;
}

//! Add data to struct
//! 
//! @param s
//!   data to add to end of struct
void add_data(string s)
{
  buffer += s;
}

//! Return all data in struct, emptying the struct
//! 
//! @returns
//!   contents of struct
string pop_data()
{
  string res = buffer;
  create();
  return res;
}

//!  Append an unsigned integer to struct
//!
//!  @param i
//!    unsigned integer to append
//!  @param len
//!    length of integer in bytes
void put_uint(int i, int len)
{
  if (i<0)
    error("ADT.struct->put_uint: negative argument.\n");
  
  add_data(sprintf("%*c", len, i));
}

//! @fixme
//!  document me!
void put_var_string(string s, int len)
{
  if ( (len <= 3) && (sizeof(s) >= ({ -1, 0x100, 0x10000, 0x1000000 })[len] ))
    error("ADT.struct->put_var_string: Field overflow.\n");
  put_uint(sizeof(s), len);
  add_data(s);
}

//! @fixme
//!  document me!
void put_bignum(object i, int|void len)
{
  if (i<0)
    error("ADT.struct->put_bignum: negative argument.\n");
  put_var_string(i->digits(256), len || 2);
}

//! @fixme
//!  document me!
void put_fix_string(string s)
{
  add_data(s);
}

//! @fixme
//!  document me!
void put_fix_uint_array(array(int) data, int item_size)
{
  foreach(data, int i)
    put_uint(i, item_size);
}

//! @fixme
//!  document me!
void put_var_uint_array(array(int) data, int item_size, int len)
{
  put_uint(sizeof(data), len);
  put_fix_uint_array(data, item_size);
}

//! @fixme
//!  document me!
int get_uint(int len)
{
  mixed i;
  if ( (sizeof(buffer) - index) < len)
    error("ADT.struct->get_uint: no data\n");
  sscanf(buffer, "%*" + (string) index +"s%" + (string) len + "c", i);
  index += len;
  return i;
}

//! @fixme
//!  document me!
string get_fix_string(int len)
{
  string res;
  
  if ((sizeof(buffer) - index) < len)
    error("ADT.struct->get_fix_string: no data\n");
  res = buffer[index .. index + len - 1];
  index += len;
  return res;
}

//! @fixme
//!  document me!
string get_var_string(int len)
{
  return get_fix_string(get_uint(len));
}

#if constant(Gmp.mpz)
object get_bignum(int|void len)
{
  return Gmp.mpz(get_var_string(len || 2), 256);
}
#endif

//! get remainder of data from struct, clearing the struct
//! 
//! @returns
//!   Remaining contents of struct
string get_rest()
{
  string s = buffer[index..];
  create();
  return s;
}

//! @fixme
//!  document me!
array(mixed) get_fix_uint_array(int item_size, int size)
{
  array(mixed) res = allocate(size);
  for(int i = 0; i<size; i++)
    res[i] = get_uint(item_size);
  return res;
}

//! @fixme
//!  document me!
array(mixed) get_var_uint_array(int item_size, int len)
{
  return get_fix_uint_array(item_size, get_uint(len));
}

//! is struct empty?
//!
//! @returns
//!   1 if empty, 0 otherwise
int is_empty()
{
  return (index == sizeof(buffer));
}

