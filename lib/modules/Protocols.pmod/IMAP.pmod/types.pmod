/* IMAP.types
 *
 */

string imap_format(mixed x)
{
  if (!x)
    return "nil";
  else if (stringp(x))
    return x;
  else return x->format();
}

string imap_format_array(array a)
{
  return Array.map(a, imap_format) * " ";
}

/* Output types */
class imap_atom
{
  string name;

  void create(string s) { name = s; }

  string format() { return name; }
}

class imap_string
{
  string data;

  void create(string s) { data = s; }

  string format()
    {
      if (!sizeof(array_sscanf(data, "%*[^\0-\037\177-\377]%s")[0]))
	return "\"" + replace(data, ({ "\"", "\\" }), ({ "\\\"", "\\\\" }) ) + "\"";
      else
	return sprintf("{%d}\r\n%s", strlen(data), data);
    }
}

class imap_list
{
  array data;

  void create(array a) { data = a; }

  string format() { return "(" + imap_format_array(data) + ")"; }
}

class imap_prefix
{
  array data;

  void create(array a) { data = a; }

  string format() { return "[" + imap_format_array(data) + "]"; }
}

class imap_number
{
  int value;

  void create(int n) { value = n; }

  string format() { return sprintf("%d", value); }
}
