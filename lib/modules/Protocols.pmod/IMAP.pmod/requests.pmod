/* IMAP.requests
 *
 */

import .types;

#if 0
class request
{
  string tag;
  object line;
  
  void create(string t, object l)
    {
      tag = t;
      line = l;
    }

  string process(object|mapping session, object server, function send)
    {
      send(tag, "OK");
      return "finished";
    }

#if 0
  string format(string tag, object|string ... args)
    {
      return tag + " "
	+ Array.map(args, lambda (mixed x)
			    {
			      if (stringp(x))
				return x;
			      else
				return x->format();
			    })
	* " " + "\r\n";
    }

  string untagged_response(object|string ... args)
    {
      return format("*", @args);
    }
  
  string response_ok(object|string ... args)
    {
      format(tag, "OK", @args);
    }

  string response_no(object|string ... args)
    {
      format(tag, "NO", @args);
    }

  string response_BAD(object|string ... args)
    {
      format(tag, "BAD", @args);
    }
#endif
}
#endif

class Parser
{
  object line; /* Current line */

  void create(object l)
    {
      line = l;
    }

  /* These functions are all called directly or indirectly by the
   * request enging in imap_server.pike. When a complete value is
   * ready, they call a continuation function given as an argument.
   * They all return an action mapping, saying what to do next. */
  
  /* First a few relatively simple functions */

  /* Used when line == 0, i.e. to continue reading after a literal */
  class line_handler
  {
    function process;
    array args;
    
    void create(function p, mixed ... a)
      {
	process = p;
	args = a;
      }

    mapping `()(object l)
      {
	line = l;
	return process(@a);
      }
  }

  /* Value is an integer */
  mapping get_number()
    {
      if (line)
	return c(line->get_number());

      return ([ "action" : "expect_line",
		"handler" : line_handler(get_number, c) ]);
    }
  
  /* Value is a string */
  mapping get_atom(c)
    {
      if (line)
	return c(line->get_atom());

      return ([ "action" : "expect_line",
		"handler" : line_handler(get_atom, c) ]);
    }

  class get_string_handler
  {
    function c;
    
    void create(function _c)
      {
	c = _c;
      }

    mapping `()(string l)
      {
	return c(s);
      }
  }

  /* Value is a string */
  mapping get_string(c)
    {
      if (line)
      {
	string|object s = line->get_string();
	if (stringp(s))
	  return c(s);

	line = 0;
	return ([ "action", "expect_literal",
		  "length", s->length,
		  "handler" : get_string_handler(c) ]);
      }
      return ([ "action" : "expect_line",
		"handler" : line_handler(get_string, c) ]);
    }

  mapping get_astring(c)
    {
      if (line)
      {
	string|object s = line->get_astring();
	if (stringp(s))
	  return c(s);

	line = 0;
	return ([ "action", "expect_literal",
		  "length", s->length,
		  "handler" : get_string_handler(c) ]);
      }
      return ([ "action" : "expect_line",
		"handler" : line_handler(get_astring, c) ]);
    }

  /* VAlue is a set object */
  mapping get_set
    {
      if (line)
	return c(line->get_set());

      return ([ "action" : "expect_line",
		"handler" : line_handler(get_set, c) ]);
    }
  
  /* The values produced by these functions are mappings, all of which
   * includes a type field. */

  class handle_literal
  {
    function handler;
  
    void create(function h)
      {
	handler = h;
      }
    mixed `()(string s)
      {
	return handler( ([ "type" : "string", "string" : s ]) );
      }
  }

  /* The function c is called with the resulting mapping and the
   * optional arguments. */
  
  class handle_list
  {
    function c;
  
    void create(function _c)
      {
	c = _c;
      }

    mixed `()(array l)
      {
	return c( ([ "type" : "list", "list" : l ]) );
      }
  }

  class collect_list
  {
    array l = ({ });

    int max_depth;
    int eol;

    function c;

    void create(int _max_depth, int _eol, function _c)
      {
	max_depth = _max_depth;
	eol = _eol;
	c = _c;
      }
    
    mapping append(mapping value)
      {
	if (value->eol)
	  return c(l);
	l += ({ value });

	return collect();
      }
    
    mapping collect()
      {
	return get_any(max_depth, eol, append);
      }
  }

  /* Atoms with an option list */
  class handle_options
  {
    function c;
    mapping value;
    
    void create(mapping v, function _c)
      {
	value = v;
	c = _c;
      }

    mixed `()(array l)
      {
	value->options = l;
	/* line cannot be NULL, as the last token parsed is always ']',
	 * not a literal */
	return c(line->get_range(value));
      }
  }

  mapping get_any(int max_depth, int eol, function c)
    {
      if (!line)
	return ([ "action" : "expect_line",
		  "handler" : line_handler(get_any, max_depth, eol, c) ]);

      mapping t = line->get_token(eol);

      if (!t)
	return c(t);
  
      switch(t->type)
      {
      case "atom":
      case "string":
      case "eol":
	return c(t);
      case "literal":
	return ([ "action" : "expect_literal",
		  "length" : t->length,
		  "handler": handle_literal(c) ]);
      case "atom_options":
	if (max_depth > 0)
	  return collect_list(max_depth - 1, ']', handle_options(t, c))->collect();
	else return 0;
      case "list":
	return collect_list(max_depth - 1, ')', handle_list(c))->collect();
      default:
	throw( ({ "IMAP: Internal error!\n", backtrace() }) );
      }
    }
}
  
class Request
{
  string tag;
  object parser;
  object io;
  object server;
  object session;
  
  void create(string t, object l)
    {
      tag = t;
      parser = Parser(line);

      args = allocate(sizeof(arg_info));
      argc = 0;
    }

  constant arg_info = ({ });

  string easy_process(object|mapping session, object server,
		      function send, mixed ... args);
  
  array args;
  int argc;

  string process_literal(string literal, object|mapping session, object server,
			 object io)
    {
      args[argc++] = literal;
      if (argc == sizeof(args))
	return easy_process(session, server, send, @args);
      else
	return ([ "action" : "expect_line",
		  "handler" : process_line ]);
    }

  string process_line(object l, object|mapping session, object server,
		      object io)
    {
      line = l;
      return process(session, server, io);
    }

  string get_literal(object io)
    {
      io->send_imap("+", ( (sizeof(arg_info[argc]) > 1)
			   ? arg_info[argc][1] : "Ready") );
      return ([ "action" : "expect_literal",
		"length" : args[argc]->length,
		"handler": process_literal,
      ]);
    }
  
  mapping process(object|mapping s, object db, object o)
    {
      session = s;
      server = db;
      io = o;

      return collect_args();
    }

  mapping append_number(int i)
    {
      if (i<0)
	return ([ "action" : "bad",
		  "msg" : "Invalid number" ]);

      args[argc++] = i;
      return collect_args();
    }

  mapping append_arg(mixed o)
    {
      if (!o)
	return ([ "action" : "bad",
		  "msg" : "Invalid or missing argument" ]);

      args[argc++] = o;
      return collect_args();
    }
  
  mapping collect_args
    {
      if (argc == sizeof(args))
	return easy_process(@args);	

      switch(arg_info[argc][0])
      {
      case "number":
	return parser->get_number(append_number);
	
      case "string":
	return parser->get_astring(append_arg);

      case "astring":
	return parser->get_astring(append_arg);
      case "set":
	return parser->get_set(append_arg);
      case "any":
	/* A single atom or string or a list of atoms (with
	 * options), lists. Used for fetch. */
	return parser->get_any(append_arg);
      default:
	throw( ({ sprintf("IMAP.requests: Unknown argument type %O\n",
			  arg_info[argc]), backtrace() }) );
      }
    }

#if 0
  string process(object|mapping session, object server, function send)
    {
      while(argc < sizeof(args))
      {
	switch(arg_info[argc][0])
	{
	case "number":
	  if ( (args[argc++] = line->get_number()) < 0)
	  {
	    send(tag, "BAD", "Invalid number");
	    return "finished";
	  }
	  break;
	case "string":
	  if (!(args[argc] = line->get_string()))
	  {
	    send(tag, "BAD", "Missing or invalid argument");
	    return "finished";
	  }
	  if (objectp(args[argc]))
	    return request_literal(send);
	  argc++;
	  break;
	case "astring":
	  if (!(args[argc] = line->get_astring()))
	  {
	    send(tag, "BAD", "Missing or invalid argument");
	    return "finished";
	  }
	  if (objectp(args[argc]))
	    return request_literal(send);
	  argc++;
	  break;
	case "set": 
	  if (!(args[argc] = line->get_set()))
	  {
	    send(tag, "BAD", "Invalid set");
	    return "finished";
	  }
	  argc++;
	  break;
	case "simple_list":
	  	  if (!(args[argc] = line->get_set()))
	  {
	    send(tag, "BAD", "Invalid set");
	    return "finished";
	  }
	  argc++;
	  break;
	  
	default:
	  throw( ({ sprintf("IMAP.requests: Unknown argument type %O\n",
			    arg_info[argc]), backtrace() }) );
	}
      }
      return easy_process(session, server, send, @args);
    }
#endif
  void create(string tag, object line)
    {
      ::create(tag, line);
      args = allocate(sizeof(arg_info));
      argc = 0;
    }
}


class noop
{
  inherit request;

  string process(object session, object server, function send)
    {
      array status = server->update(session);
      
      if (status)
	foreach(status, array a)
	  send("*", @a);
      
      send(tag, "OK");
      return "finished";
    }
}

class capability
{
  inherit request;

  string process(object|mixed session, object server, function send)
    {
      send("*", "CAPABILITY", @server->capabilities(session));
      send(tag, "OK");
      return "finished";
    }
}

class login
{
  inherit easy_request;

  constant arg_info = ({
    ({ "astring", "Ready for user name" }),
    ({ "astring", "Ready for pass word" }) });
  
  mixed uid;
  mixed get_uid() { return uid; }
  
  string easy_process(object|mapping session, object server,
		      function send, string name, string passwd)
    {
      /* Got name and passwd. Attempt authentication. */
      uid = server->login(session, name, passwd);
      
      if (!uid)
      {
	send(tag, "NO");
	return "finished";
      }
      send(tag, "OK");
      return "login";
    }
}

class logout
{
  inherit request;

  string process(object|mapping session, object server, function send)
    {
      send("*", "BYE");
      send(tag, "OK");
      return "close";
    }
}

class list
{
  inherit easy_request;
  constant arg_info = ({ ({ "astring", "Ready for mailbox name" }),
			 ({ "astring" }) });

  string easy_process(object|mapping session, object server,
		      function send, string reference, string glob)
    {
      /* Each element of the array should be an array with three elements,
       * attributes, hierarchy delimiter, and the name. */

      if ( (reference == "")
	   && (lower_case(glob) == "inbox") )
	glob = "INBOX";
      
      array mailboxes = server->list(session, reference, glob);
      
      if (mailboxes)
	foreach(mailboxes, array a)
	  send("*", @a);
      
      send(tag, "OK");
      return "finished";
    }
}

class lsub
{
  inherit easy_request;
  constant arg_info = ({ ({ "astring", "Ready for mailbox name" }),
			 ({ "astring" }) });

  string easy_process(object|mapping session, object server,
		      function send, string reference, string glob)
    {
      /* Each element of the array should be an array with three elements,
       * attributes, hierarchy delimiter, and the name. */

      if ( (reference == "")
	   && (lower_case(glob) == "inbox") )
	glob = "INBOX";
      
      array mailboxes = server->lsub(session, reference, glob);
      
      if (mailboxes)
	foreach(mailboxes, array a)
	  send("*", @a);
      
      send(tag, "OK");
      return "finished";
    }
}

class select
{
  inherit easy_request;

  constant arg_info = ({ ({ "astring" }) });

  string easy_process(object|mapping session, object server,
		      function send, string mailbox)
    {
      if (lower_case(mailbox) == "inbox")
	mailbox = "INBOX";

      array info = server->select(session, mailbox);

      if (info)
      {
	foreach(info, array a)
	  send("*", @a);
	send(tag, "OK", imap_prefix( ({ "READ-WRITE" }) ) );
	return "select";
      } else {
	send(tag, "NO");
	return "login";
      }
    }
}

class fetch
{
  inherit easy_request;

  constant arg_info = ({ ({ "set" }), ({ "simple_list"}) });

  string easy_process(object|mapping session, object server,
		      function send,
		      object message_set, mapping request)
    {
      array fetch_attrs = 0;
      switch(request->type)
      {
      case "atom":
	/* Short hands */
	if (!request->options)
	  /* No attributes except BODY and BODY.PEEK can take any options */
	  switch(lower_case(request->atom))
	  {
	  case "all":
	    fetch_attrs = ({ "flags", "internaldate",
			     "rfc822.size", "envelope" });
	    break;
	  case "fast":
	    fetch_attrs = ({ "flags", "internaldate", "rfc822.size" });
	    break;
	  case "full":
	    fetch_attrs = ({ "flags", "internaldate",
			     "rfc822.size", "envelope", "body" });
	    break;
	  default:
	    /* Handled below */
	  }
	if (!fetch_attrs)
	{
	  mixed f = process_fetch_attr(request);
	  if (!f)
	  {
	    send(tag, "BAD");
	    return "finished";
	  }
	  fetch_attrs = ({ f });
	}
      case "list":
	fetch_attrs = allocate(sizeof(request->list));

	for(int i = 0; i<sizeof(fetch_attrs); i++)
	{
	  if (!(fetch_attrs[i] = process_fetch_attr(request->list[i])))
	  {
	    send(tag, "BAD");
	    return "finished";
	  }
	}
	break;
      default:
	throw( ({ "Internal error!\n", backtrace() }) );
      }

      array info;

      /* If the arguments are invalid, fetch() can throw a string,
       * which is returned to the client as an error message. */
      mixed e = catch {
	info = server->fetch(session, message_set, fetch_attrs);
      };

      if (e)
      {
	if (stringp(e))
	{
	  send(tag, "BAD", e);
	  return "finished";
	}
	else throw(e);
      }
      if (info)
      {
	foreach(info, array a)
	  send("*", @a);
	send(tag, "OK");
      } else 
	send(tag, "NO");

      return "finished";
    }

  mapping process_fetch_attr(mapping atom)
    {
      if (atom->type != atom)
	return 0;

      string wanted = lower_case(atom->atom);
      mapping res = ([ "wanted" : wanted ]);

      /* Should requesting any part of the body realy vount as reading it? */
      if ( (< "body", "rfc822", "rfc822.text" >) [wanted])
	res->mark_as_read = 1;

      switch(wanted)
      {
      case "body":
	if (!atom->options)
	{
	  res->wanted = "bodystructure";
	  res->no_extention_data = 1;
	  return res;
	}
	/* Fall through */
      case "body.peek":
	if (!atom->options)
	  return 0;
	
	if (sizeof(atom->options)
	    && ( (atom->options[0]->type != atom)
		 || (atom->options[0]->options)))
	  return 0;
	
	array path = atom->options[0]->atom / ".";

	/* Extract numeric prefix */
	array part_number = ({ });
	int i;

	for(i = 0; i<sizeof(path); i++)
	{
	  int n = string_to_number(path[i]);
	  if (n<0)
	    break;
	  part_number += ({ n });
	}

	res->section = path[i..];
	res->part = part_number;
	res->options = atom->options[1..];
	res->partial = atom->range;

	return res;
      default:
	/* Handle below */
      }
      
      /* No commands except BODY[.PEEK] accepts any options */
      if (atom->options)
	return 0;

      return (< "envelope",
		"flags",
		"internaldate",
		"rfc822.header", "rfc822.size", "rfc822.text",
		"bodystructure",
		"uid" >)[wanted]
	&& res;
    }
}
