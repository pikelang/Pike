/* IMAP.server
 *
 */

class partial_literal
{
  int length;

  void create(int l) { length = l; }
}
    
class line_buffer
{
  string buffer;

  void create(string line)
    {
      buffer = line;
    }

  void skip_whitespace()
    {
      sscanf(buffer, "%*[ \t]%s", buffer);
    }
  
  // Returns -1 on error. All valid numbers ar non-negative.
  int get_number()
    {
      skip_whitespace();
      
      if (!strlen(buffer))
	return -1;

      int i;
      for(i = 0; i<strlen(buffer); i++)
	if ( (buffer[i] < '0') || ('9' < buffer[i]) )
	  break;
      
      // Disallow too large numbers
      if (!i || (i > 9) )
	return -1;
      
      int res = array_sscanf(buffer[..i-1], "%d")[0];
      buffer = buffer[i..];
      return res;
    }
  
  string get_atom()
    {
      string atom;

      werror("get_atom: buffer = '%s'\n", buffer);
      sscanf(buffer, "%*[ \t]%[^(){ \0-\037\177%*\"\\]%s", atom, buffer);

      if (strlen(buffer)
	  && (buffer[0] != ' ')
	  && (buffer[0] != '\t'))
	return 0;
      
      return strlen(atom) && atom;
    }

  string|object get_string()
    {
      werror("get_string: buffer = '%s'\n", buffer);

      skip_whitespace();

      if (!strlen(buffer))
	return 0;
      
      switch(buffer[0])
      {
      case '"':
      {
	int i = 0;
	while(1)
	{
	  i = search(buffer, "\"", i+1);
	  if (i<0)
	    return 0;
	  if (buffer[i-1] != '\\')
	  {
	    string res = replace(buffer[1..i-1],
				 ({ "\\\"", "\\\\" }),
				 ({ "\"", "\\" }) );
	    buffer = buffer[i+1..];
	    return res;
	  }
	}
      }
      case '{':
      {
	if (buffer[strlen(buffer)-1..] != "}")
	  return 0;
	string n = buffer[1..strlen(buffer)-2];

	buffer = "";
	if ( (sizeof(values(n) - values("0123456789")))
	     || (sizeof(n) > 9) )
	  return 0;
	if (!sizeof(n))
	  return 0;

	return partial_literal(array_sscanf(n, "%d")[0]);
      }
      }
    }

  string|object get_astring()
    {
      werror("get_astring: buffer = '%s'\n", buffer);
      
      skip_whitespace();
      if (!strlen(buffer))
	return 0;
      switch(buffer[0])
      {
      case '"':
      case '{':
	return get_string();
      default:
	return get_atom();
      }
    }

#if 0
  array get_list()
    {
    }
#endif
}

class read_buffer
{
  string buffer = "";
  
  void add_data(string s)
    {
      buffer += s;
    }

  string get_raw_line()
    {
      int i = search(buffer, "\r\n");
      if (i<0)
	return 0;

      string res = buffer[..i-1];
      buffer = buffer[i+2..];
      
      return res;
    }

  object get_line()
    {
      string l = get_raw_line();
      return l && line_buffer(l);
    }
  
  string get_literal(int length)
    {
      if (strlen(buffer) < length)
	return 0;

      string res = buffer[..length-1];
      buffer = buffer[length..];

      return res;
    }
}

mapping unauth_commands =
([ "noop" : .requests.noop,
   "capability" : .requests.capability,
   "logout" : .requests.logout,
   "login" : .requests.login
   // , "authenticate" : .requests.authenticate
]);

mapping auth_commands =
([ "noop" : .requests.noop,
   "logout" : .requests.logout,
   "capability" : .requests.capability,
   "select" : .requests.select,
//   "examine" : .requests.examine,
//   "create" : .requests.create,
//   "delete" : .requests.delete,
//   "rename" : .requests.rename,
//   "subscribe" : .requests.subscribe,
//   "unsubscribe" : .requests.unsubscribe,
   "list" : .requests.list,
   "lsub" : .requests.lsub //,
//   "status" : .requests.status,
//   "append" : .requests.append
]);

mapping select_commands =
([ "noop" : .requests.noop,
   "logout" : .requests.logout,
   "capability" : .requests.capability,
   "select" : .requests.select,
//    "examine" : .requests.examine,
//    "create" : .requests.create,
//    "delete" : .requests.delete,
//    "rename" : .requests.rename,
//    "subscribe" : .requests.subscribe,
//    "unsubscribe" : .requests.unsubscribe,
   "list" : .requests.list,
   "lsub" : .requests.lsub //,
//    "status" : .requests.status,
//    "append" : .requests.append,
//    "check" : .requests.check,
//    "close" : .requests.close,
//    "expunge" : .requests.expunge,
//    "search" : .requests.search,
//    "fetch" : .requests.fetch,
//    "store" : .requests.store,
//    "copy" : .requests.copy,
//    "uid" : .requests.uid
]);

mapping all_commands =
([ "initial" : ([ ]),
   "non_authenticated" : unauth_commands,
   "authenticated" : auth_commands,
   "selected" : select_commands,
   "closed": ([ ]) ]);

object buffer = read_buffer();
string state;

int debug_level;

void create(string start_state, int|void debug)
{
  state = start_state;
  debug_level = debug;
}

void add_data(string s) { buffer->add_data(s); }

#if 0
/* FIXME: Send a BAD response to the client */
void protocol_error(string message)
{
  if (debug_level)
    werror("protocol error\n");
  throw( ({ sprintf("IMAP.pmod: protocol error %s\n", message),
	    backtrace() }) );
}
#endif

class bad_request
{
  string tag;
  string msg;
  
  void create(string t, string m)
    {
      tag = t || "*";
      msg = m;
    }
  
  string process(object|mapping session, object server, function send)
    {
      send(tag, "BAD", msg);
      return "finished";
    }
}

object get_request()
{
  object line = buffer->get_line();
  
  if (!line)
    return 0;

  if (debug_level)
    werror("Read line: '%s'\n", line->buffer);
  
  string tag = line->get_atom();
  
  if (!tag)
  {
    return bad_request(0, "No tag");
  }
  
  string command = line->get_atom();

  if (!command)
  {
    return bad_request(tag, "No command");
  }

  if (debug_level)
    werror("Read command: '%s'\n", command);
    
  function req = all_commands[state][lower_case(command)];

  if (!req)
  {
    return bad_request(tag, "Unknown command");
  }

  // FIXME: How should invalid requests be handled?
  return req(tag, line);
}

