/* IMAP.types
 *
 */

#pike __REAL_VERSION__

string imap_format(mixed x)
{
  if (!x)
    return "NIL";
  else if (stringp(x))
    return x;
  else return x->format();
}

string imap_format_array(array a)
{
#if 0
  if (!sizeof(a)) {
    return "";
  }
  a = Array.map(a, imap_format);
  string res = a[0];
  for(int i=1; i < sizeof(a); i++) {
    if (sizeof(res) && (res[-1] == '\n')) {
      // Special case - No space needed.
      res += a[i];
    } else {
      res += " " + a[i];
    }
  }
  return res;
#else /* !0 */
  return Array.map(a, imap_format) * " ";
#endif /* 0 */
}

array imap_check_array(array a)
{
  return Array.filter(a, lambda(mixed item, array index) {
			   index[0]++;
			   if (!item || stringp(item) || objectp(item)) {
			     return 1;
			   }
			   werror(describe_backtrace( ({
			     sprintf("Bad array element %O (%d)\n",
				     item, index[0]),
			     backtrace()
			   }) ));
			   return 0;
			 }, ({ -1 }) );
}

/* Output types */
class imap_atom
{
  string name;

  void create(string s) { name = s; }

  string format() { return name; }
}

class imap_atom_options
{
  string name;
  array options;
  array range;

  void create(string s, string sec, array o, array r)
    {
      name = s;
      options = ({ sec,
		   o && sizeof(o) &&
		   imap_list(Array.map(imap_check_array(o[0]->list->atom),
				       imap_string)) }) - ({ 0 });
      range = r;
    }

  string format()
    {
      werror(sprintf("options:%O\n", options));
      return upper_case(name + "[" + Array.map(options, imap_format)*" " + "]")
	// NOTE: Only the start index is sent
	+ (range ? sprintf("<%d>", range[0]) : "");
    }
}

class imap_string
{
  string data;

  void create(string s) { data = s; }

  string format()
    {
      // Pine doesn't know about \-quoting...
      if (!sizeof(array_sscanf(data, "%*[^\0-\037\\\"\177-\377]%s")[0]))
	return "\"" + replace(data, ({ "\"", "\\" }), ({ "\\\"", "\\\\" }) ) + "\"";
      else
	return sprintf("{%d}\r\n%s", sizeof(data), data);
    }
}

class imap_list
{
  array data;

  void create(array a) { data = imap_check_array(a); }

  string format() { return "(" + imap_format_array(data) + ")"; }
}

class imap_prefix
{
  array data;

  void create(array a) { data = imap_check_array(a); }

  string format() { return "[" + imap_format_array(data) + "]"; }
}

class imap_number
{
  int value;

  void create(int n) { value = n; }

  string format() { return sprintf("%d", value); }
}

/* Input types */

// Returns -1 on error. 
int string_to_number(string s)
{
  if (!sizeof(s) || (sizeof(s)  > 9))
    return -1;
  if (sizeof(values(s) - ({ '0', '1', '2', '3', '4',
			    '5', '6', '7', '8', '9' })))
    return -1;
  return array_sscanf(s, "%d")[0];
}


class imap_set
{
  /* Each element is either an integer, the string "*", or an
   * array ({ start, end }) */
  array items;

  void create(array|void a)
    {
      items = a;
    }
  
  int|string string_to_number_star(string s)
    {
      return (s == "*") ? s : string_to_number(s);
    }
  
  int|string|array string_to_subset(string s)
    {
      int i = search(s, ":");

      if (i<0)
	return string_to_number_star(s);

      int|string start = string_to_number_star(s[..i-1]);
      if (start < 0)
	return -1;

      int end = string_to_number_star(s[i+1..]);
      if (intp(end) && (end < 0))
	return -1;
      if (end == start)
	return start;

      return ({ start, end });
    }

  object init(string s)
    {
      items = s/",";

      for(int i = 0; i<sizeof(items); i++)
      {
	items[i] = string_to_subset(items[i]);
	
	if (intp(items[i]) && (items[i]<0))
	  return 0;
      }
      return this_object();
    }

  int replace_number(int|string i, int largest)
    { return (i == "*") ? largest : i; }

  array(int) expand_item(int|string|array item, int largest)
    {
      if (arrayp(item))
      {
	int start = replace_number(item[0], largest);
	int end = replace_number(item[1], largest);

	if (end > largest)
	  end = largest;

	if (end < start)
	  return ({ });
	
	array res = allocate(end - start + 1);
	for(int i = start; i<= end; i++)
	  res[i-start] = i;

	return res;
      }
      int i = replace_number(item, largest);
      return (i > largest) ? ({ }) : ({ i });
    }
      
  /* Return a plain array of integers. LARGEST is the number to be
   * substituted for "*". Handles empty intervals, and cuts away
   * elements larger than LARGEST. Does not attempt to handle
   * overlapping intervals intelligently. */
  array(int) expand(int largest)
    {
      return `+( ({}), @Array.map(items, expand_item, largest));
    }
}
  
