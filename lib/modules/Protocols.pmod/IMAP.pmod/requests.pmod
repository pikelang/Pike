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
  
class request
{
  string tag;
  object parser;
  object io;
  object server;
  object session;
  
  void create(string t, object l)
    {
      tag = t;
      parser = .parser(line);

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
