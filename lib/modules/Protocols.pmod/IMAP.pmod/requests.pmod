/* IMAP.requests
 *
 */

import .types;
  
class request
{
  string tag;
  object parser;
  object server;
  mapping session;

  function send;

  
  void create(string t, object line)
    {
      tag = t;
      parser = .parser(line);

      args = allocate(sizeof(arg_info));
      argc = 0;
    }

  constant arg_info = ({ });

  mapping easy_process(mixed ... args);
  
  array args;
  int argc;
  
  mapping process(mapping s, object db, function io)
    {
      session = s;
      server = db;
      send = io;
      
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
  
  mapping collect_args()
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
	return parser->get_any(arg_info[argc][1], 0, append_arg, );
      default:
	throw( ({ sprintf("IMAP.requests: Unknown argument type %O\n",
			  arg_info[argc]), backtrace() }) );
      }
    }
}

class noop
{
  inherit request;
  constant arg_info = ({ });

  mapping easy_process()
    {
      array status = server->update(session);
      
      if (status)
	foreach(status, array a)
	  send("*", @a);
      
      send(tag, "OK");
      
      return ([ "action" : "finished" ]);
    }
}

class capability
{
  inherit request;
  constant arg_info = ({ });
  
  mapping easy_process()
    {
      send("*", "CAPABILITY", @server->capabilities(session));
      send(tag, "OK");
      return ([ "action" : "finished" ]);
    }
}

class login
{
  inherit request;
  constant arg_info = ({
    ({ "astring", "Ready for user name" }),
    ({ "astring", "Ready for pass word" }) });
  
  mixed uid;
  mixed get_uid() { return uid; }
  
  mapping easy_process(string name, string passwd)
    {
      /* Got name and passwd. Attempt authentication. */
      uid = server->login(session, name, passwd);
      
      if (!uid)
      {
	send(tag, "NO");
	return ([ "action" : "finished" ]);
      }
      send(tag, "OK");
      return ([ "action" : "logged_in_state" ]);
    }
}

class logout
{
  inherit request;
  constant arg_info = ({ });
  
  mapping easy_process()
    {
      send("*", "BYE");
      send(tag, "OK");
      return ([ "action" : "close" ]);
    }
}

class list
{
  inherit request;
  constant arg_info = ({ ({ "astring", "Ready for mailbox name" }),
			 ({ "astring" }) });

  mapping easy_process(string reference, string glob)
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
      return ([ "action" : "finished" ]);
    }
}

class lsub
{
  inherit request;
  constant arg_info = ({ ({ "astring", "Ready for mailbox name" }),
			 ({ "astring" }) });

  mapping easy_process(string reference, string glob)
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
      return ([ "action" : "finished" ]);
    }
}

class select
{
  inherit request;
  constant arg_info = ({ ({ "astring" }) });

  mapping easy_process(string mailbox)
    {
      if (lower_case(mailbox) == "inbox")
	mailbox = "INBOX";

      array info = server->select(session, mailbox);

      if (info)
      {
	foreach(info, array a)
	  send("*", @a);
	send(tag, "OK", imap_prefix( ({ "READ-WRITE" }) ) );
	return ([ "action" : "selected_state" ]);
      } else {
	send(tag, "NO");
	return ([ "action" : "logged_in_state" ]);
      }
    }
}

class fetch
{
  inherit request;
  constant arg_info = ({ ({ "set" }), ({ "any", 3 }) });

  mapping easy_process(object message_set, mapping request)
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
	    return ([ "action" : "finished" ]);
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
	    return ([ "action" : "finished" ]);
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
	  return ([ "action" : "bad", "msg" : e ]);
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

      return ([ "action" : "finished" ]);
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
