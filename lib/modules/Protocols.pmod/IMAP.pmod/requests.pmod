/* IMAP.requests
 *
 * $Id: requests.pmod,v 1.29 1999/02/09 21:39:09 grubba Exp $
 */

import .types;

// FIXME: Pass the current request's tag when returning a "bad" action.

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

  mapping bad(string msg)
    {
      return ([ "action" : "bad", "tag" : tag, "msg" : msg]);
    }
  
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
	return bad("Invalid or missing argument");

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
	return parser->get_any(arg_info[argc][1], 0, 0, append_arg);

      case "varargs":
	/* Like any, but with an implicit list at top-level */
	return parser->get_varargs(arg_info[argc][1], 0, append_arg);

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
    werror(sprintf("fetch->easy_process(X, %O)\n", request));
    array fetch_attrs = 0;
    switch(request->type)
    {
    case "atom":
      /* Short hands */
      if (!request->options)
	/* No attributes except BODY and BODY.PEEK can take any options */
	switch(lower_case(request->atom))
	{
#define ATTR(x) ([ "wanted" : (x) ])
#define ATTR_SECTION(x,y) ([ "wanted" : (x), "section" : (y) ])
	case "all":
	  fetch_attrs = ({ ATTR("flags"), ATTR("internaldate"),
			   ATTR_SECTION("rfc822", ({ "size" })),
			   ATTR("envelope") });
	  break;
	case "fast":
	  fetch_attrs = ({ ATTR("flags"), ATTR("internaldate"),
			   ATTR_SECTION("rfc822", ({ "size" })), });
	  break;
	case "full":
	  fetch_attrs = ({ ATTR("flags"), ATTR("internaldate"),
			   ATTR_SECTION("rfc822", ({ "size" })),
			   ATTR("envelope"),
			   ([ "wanted" : "bodystructure",
			      "no_extention_data" : 1 ]) });
	  break;
#undef ATTR
#undef ATTR_SECTION
	default:
	  /* Handled below */
	}
      if (!fetch_attrs)
      {
	mixed f = process_fetch_attr(request);
	if (!f)
	{
	  return bad("Invalid fetch");
	}
	fetch_attrs = ({ f });
      }
      break;	/* FIXME: Was FALL_THROUGH */
    case "list":
      fetch_attrs = allocate(sizeof(request->list));
      
      for(int i = 0; i<sizeof(fetch_attrs); i++)
      {
	if (!(fetch_attrs[i] = process_fetch_attr(request->list[i])))
	{
	  return bad("Invalid fetch");
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
	return bad(e);
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
    werror("fetch->process_fetch_attr(%O)\n", atom);

    if (atom->type != "atom")
      return 0;

    string wanted = lower_case(atom->atom);
    mapping res = ([ "wanted" : wanted ]);

    /* Should requesting any part of the body really count as reading it? */
    if ( (< "body", "rfc822", "rfc822.text" >) [wanted])
      res->mark_as_read = 1;

    switch(wanted)
    {
    case "body":
      if (!atom->options)
      {
	res->wanted = "bodystructure";
	res->raw_wanted = "body";  // What to say in the response
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

      res->raw_options = atom->options;
      res->section = path[i..];
      res->part = part_number;
      res->options = atom->options[1..];
      res->partial = atom->range;

      return res;
    default:
      /* Handle below */
    }

    /* No commands except BODY[.PEEK] accept any options */
    if (atom->options)
      return 0;

    array path = wanted / ".";

    if (sizeof(path) == 1)
      return (< "envelope",
		"flags",
		"internaldate",
		// "rfc822.header", "rfc822.size", "rfc822.text",
		"bodystructure",
		"uid" >)[wanted]
	&& res;

    res->raw_wanted = wanted;
      
    res->wanted = path[0];
    res->section = path[1..];
    return (res->wanted == "rfc822") && res;
  }
}

class search
{
  inherit request;
  constant arg_info = ({ ({ "varargs", 17 }) });

  mapping easy_process(array args)
    {
      string charset = "us-ascii";
	
      if (!sizeof(args))
	return bad("No arguments to SEARCH");

      if (lower(args[0]->atom) == "charset")
      {
	if ( (sizeof(args) < 2)
	     || !(charset = astring(args[1])) )
	  return bad("Bad charset to SEARCH");

	args = args[2..];
      }

      mapping criteria = parse_criteria(args)->parse_toplevel();
      
      if (!criteria)
	return bad("Invalid search criteria");
      
      array matches = server->search(session, charset, criteria);
      
      if (matches)
      {
	send("*", "SEARCH", @matches);
    	send(tag, "OK");
      }  
      else
    	send(tag, "NO");

      return ([ "action" : "finished" ]);
    }
  
  // Like lower_case(), but allows zeros
  string lower(string s)
    {
      return s && lower_case(s);
    }

  string astring(mapping m)
    {
      return m->atom || m["string"];
    }

  /* Parse the arguments to search */
  class parse_criteria
  {
    array input;
    int i;

    void create(array a)
      {
	input = a;
	i = 0;
      }

    mapping make_intersection(array criteria)
      {
	if (!criteria)
	  return 0;
      
	switch(sizeof(criteria))
	{
	case 0:
	  throw( ({ "IMAP.requests: Internal error!\n", backtrace() }) );
	case 1:
	  return criteria[0];
	default:
	  /* FIXME: Expand internal and expressions,
	   * and sort the expressions with cheapest properties first. */
	  return ([ "type" : "and", "and" : criteria ]);
	}
      }

    mapping make_union(array criteria)
      {
	if (!criteria)
	  return 0;
      
	switch(sizeof(criteria))
	{
	case 0:
	  throw( ({ "IMAP.requests: Internal error!\n", backtrace() }) );
	case 1:
	  return criteria[0];
	default:
	  /* FIXME: Expand internal or expressions. */
	  return ([ "type" : "and", "and" : criteria ]);
	}
      }

    mapping make_complement(mapping criteria)
      {
	if (!criteria)
	  return 0;
      
	return ([ "type" : "not" , "not" : criteria ]);
      }
      
    mapping get_token()
      {
	if (i == sizeof(input))
	  return 0;

	return input[i++];
      }
  
    string get_atom()
      {
	if (i == sizeof(input))
	  return 0;
	return input[i++]->atom;
      }
  
    string get_astring()
      {
	if (i == sizeof(input))
	  return 0;
      
	return astring(input[i++]);
      }

    int get_number()
      {
	if (i == sizeof(input))
	  return 0;
	i++;
	return input[i]->atom ? string_to_number(input[i]->atom) : -1;
      }

    int get_set()
      {
	if (i == sizeof(input))
	  return 0;
	i++;
	return input[i]->atom && imap_set()->init(input[i]->atom);
      }
	
    mapping parse_one()
      {
	mapping token = get_token();

	if (!token)
	  return 0;
      
	switch(token->type)
	{
	case "list":
	  return make_intersection(parse_criteria(token->list)->parse_all());
	  break;
	case "atom":
	  string key = lower_case(token->atom);
	  switch(key)
	  {
	    /* Simple criteria, with no arguments. */
	  case "all":
	  case "answered":
	  case "deleted":
	  case "flagged":
	  case "new":
	  case "old":
	  case "recent":
	  case "seen":
	  case "unanswered":
	  case "undeleted":
	  case "unflagged":
	  case "unseen":
	  case "draft":
	  case "undraft":
	    i++;
	    return ([ "type" : key, key : 1 ]);
	    /* Criteria with a date argument */
	  case "before":
	  case "on":
	  case "since":
	  case "sentbefore":
	  case "senton":
	  case "sentsince": {
	    string raw = get_astring();

	    /* Format "7-May-1998" */
	    if (!raw)
	      return 0;
	    array a = lower_case(raw) / "-";
	    if (sizeof(a) != 3)
	      return 0;
	    /* FIXME: We should use som canonical representation of dates.
	     * Perhaps daynumber since 1/1 1970 would be a good choice?
	     */
	    return ([ "type" : key, key : a ]);
	  }
 	  /* Criteria taking a string as argument */
	  case "bcc":
	  case "body":
	  case "cc":
	  case "from":
	  case "subject":
	  case "text":
	  case "to": {
	    string arg = get_astring();
	  
	    return arg && ([ "type" : key, key : arg ]);
	  }
	  /* Flag based criteria */
	  case "keyword":
	  case "unkeyword": {
	    string keyword = get_atom();

	    return keyword && ([ "type" : key, key : input[i++]->atom ]);
	  }
	  /* Other */
	  case "header": {
	    string header = get_astring();
	    if (!header)
	      return 0;
	  
	    string value = get_astring();
	  
	    return value && ([ "type" : key, "header" : header, "value" : value ]);
	  }
	  /* Criterias taking numeric values */
	  case "larger":
	  case "smaller": {
	    int value = get_number();
	    return (i >= 0) && ([ "type" : key, key : value ]);
	  }
	  case "not": 
	    return make_complement(parse_one());
	    
	  case "or": {
	    mapping c1, c2;
	    
	    return (c1 = parse_one())
	      && (c2 = parse_one())
	      && make_union( ({ c1, c2 }) );
	  }
	  default: {
	    object set = imap_set()->init(key);
	    
	    return set && ([ "type" : "set", "set" : set ]);
	  }
	  }
	default:
	  return 0;
	}
      }
  
    array parse_all()
      {
	array res = ({ });

	while(i < sizeof(input))
	{
	  mapping c = parse_one();
	  if (!c)
	    return 0;
	  res += ({ c });
	}

	return sizeof(res) && res;
      }

    mapping parse_toplevel()
      {
	array a = parse_all();
	return a && make_intersection(a);
      }
  }
}

class uid {
  inherit fetch;

  constant arg_info = ({ ({ "string" }), ({ "set" }), ({ "any", 3 }) });

  mapping easy_process(string cmd, object message_set, mapping request)
  {
    werror("uid->easy_process(%O, X, %O)\n", cmd, request);

    switch(lower_case(cmd)) {
    case "fetch":
      object local_set = server->uid_to_local(session, message_set);
      return(fetch::easy_process(local_set, request));
      break;
    case "search":
    case "copy":
    default:
      throw(({ sprintf("UID: Command %O not implemented.\n",
		       upper_case(cmd)) }));
      break;
    }
  }
}
