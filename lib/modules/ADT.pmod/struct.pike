/* $Id: struct.pike,v 1.4 1998/02/11 01:53:13 mirar Exp $
 *
 * Read and write structures from strings.
 */

string buffer;
int index;

void create(void|string s)
{
  buffer = s || "";
  index = 0;
}

void add_data(string s)
{
  buffer += s;
}

string pop_data()
{
  string res = buffer;
  create();
  return res;
}
  
void put_int(int i, int len)
{
  add_data(sprintf("%*c", len, i));
}

void put_var_string(string s, int len)
{
  put_int(strlen(s), len);
  add_data(s);
}

void put_fix_string(string s)
{
  add_data(s);
}

void put_fix_array(array(int) data, int item_size)
{
  foreach(data, int i)
    put_int(i, item_size);
}

void put_var_array(array(int) data, int item_size, int len)
{
  put_int(sizeof(data), len);
  put_fix_array(data, item_size);
}

mixed get_int(int len)
{
  mixed i;
  if ( (strlen(buffer) - index) < len)
    throw( ({ "ADT.struct->get_int: no data\n", backtrace() }) );
#if constant(Gmp.mpz)
  if (len <= 3)
  {
#endif
    sscanf(buffer, "%*" + (string) index +"s%" + (string) len + "c", i);
#if constant(Gmp.mpz)
  }
  else
    i = Gmp.mpz(buffer[index .. index+len-1], 256);
#endif
  index += len;
  return i;
}

string get_fix_string(int len)
{
  string res;
  
  if ((strlen(buffer) - index) < len)
    throw( ({ "ADT.struct->get_fix_string: no data\n", backtrace() }) );
  res = buffer[index .. index + len - 1];
  index += len;
  return res;
}

string get_var_string(int len)
{
  return get_fix_string(get_int(len));
}

array(mixed) get_fix_array(int item_size, int size)
{
  array(mixed) res = allocate(size);
  for(int i = 0; i<size; i++)
    res[i] = get_int(item_size);
  return res;
}

array(mixed) get_var_array(int item_size, int len)
{
  return get_fix_array(item_size, get_int(len));
}

int is_empty()
{
  return (index == strlen(buffer));
}
	       
#if 0

constant FIELD_int = 1;
constant FIELD_string = 2;

class field
{
  string name;  /* Name of field */
  int type;
  int len;      /* For integers: length,
		 * for strings: index to the field that holds the length */
}



array(object(field)) description;
array(function) conversions;

class parser
{
  string buffer = "";
  object info;
  int field; /* Field being read */
  object o; /* The object to fill in */
  object me;

  mapping(int:function(string, object:void)) conversions;

  create(object i)
  {
    info = i;
    o = i->prog();
    me = this_object();
  }
  
  object|string recv(string data)
  {
    buffer += data;
    while(strlen(buffer) >= info->needed_chars[field])
    {
      object err;
      if (err = info->conversions[field](me))
	return err;
      field++;
      if (field >= sizeof(info->description))
	return buffer[info->needed_chars[field-1]..];
    }
    return 0;
  }
}

array compile()
{
  conversions = 


#endif
