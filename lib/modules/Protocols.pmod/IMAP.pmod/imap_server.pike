/* imap_server.pike
 *
 */

mapping unauth_commands =
([ "noop" : .requests.noop,
   "capability" : .requests.capability,
   "logout" : .requests.logout,
   "login" : .requests.login,
   // "authenticate" : .requests.authenticate
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
   "lsub" : .requests.lsub,
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
   "lsub" : .requests.lsub,
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

class connection
{
  inherit .server;

  object db; /* Mail backend */

  mapping session = ([]); /* State information about this ession; primarily
			   * uid and mailboxid. */

  object current_request;

#if 0
  void imap_close(int|void hard)
    {
      if (hard || !strlen(write_buffer))
	fd->close();
      else
	closing = 1;
    }
#endif
  
  void show_backtrace(mixed e)
    {
      werror(describe_backtrace(e));
    }

  void next_action(mapping action)
    {
      switch(action->action)
      {
      case "close":
	/* Close connection */
	break;
      case "finished":
	/* Finished processing this request. Remain in the same state. */
	get_request();
	break;
      case "expect_line":
	/* Callback for next line recieved */
	get_line(action->handler);
	break;
      case "expect_literal":
	/* Callback for recieving a literal */
	get_literal(action->length, action->handler);
	break;
      case "logged_in_state":
	use_commands(auth_commands);
	break;
      case "selected_state":
	use_commands(select_commands);
	break;
      default:
	throw( ({ sprintf("IMAP.pmod: Internal error, action = %O\n",
			  action), backtrace() }) );
      }
    }
  
  void handle_request(object req)
    {
      mapping action;
      
      mixed e;
      if (e = catch(action = req->process(session, db, this_object())))
	{
	  show_backtrace(e);
	  send_bad_response(current_request->tag, "Internal error");
	  return;
	}
      next_action(action);
    }

  void create(object f, int timeout, object server, mapping preauth, int|void debug)
    {
      fd = f;
      db = server;

      ::create(f, timeout);

      if (preauth)
      {
	session = preauth->session;
	use_commands(auth_commands);

	imap_send("*", "PREAUTH", "IMAP4rev1", preauth->message);
      } else {
	use_commands(unauth_commands);
	imap_send("*", "OK", "IMAP4", "IMAP4rev1", "Service ready");
      }
    }
}

#if 0
class connection
{
  inherit .server;

  object fd;

  object db; /* Mail backend */

  mapping session = ([]); /* State information about this ession; primarily
			   * uid and mailboxid. */

  object current_request;
  
  string write_buffer = "";
  
  int closing; /* If non-zero, close the file after write buffer is flushed */

  void die()
    {
      fd->close();
    }
  
  void imap_write_callback(mixed id)
    {
      if (strlen(write_buffer))
      {
	int written = fd->write(write_buffer);
	if (written > 0)
	  write_buffer = write_buffer[written..];
	else if (written < 0)
	  die();
      }
      if (!strlen(write_buffer))
      {
	fd->set_write_callback(0);
	if (closing)
	{
	  fd->close();
	}
      }
    }

  void imap_write(string s)
    {
      write_buffer += s;

      if (strlen(write_buffer))
	fd->set_write_callback(imap_write_callback);
    }
  
  void imap_send_line(string s)
    {
      if(debug_level)
	werror("IMAP send line: '%s'\n", s);
      
      imap_write(s + "\r\n");
    }

  void imap_send(string|object ...args)
    {
      /* FIXME: This code sends an \r\n-pair even if the last argument
       * is sent as a literal. */
      imap_send_line(.types.imap_format_array(args));
      
      if (strlen(write_buffer))
	fd->set_write_callback(imap_write_callback);
    }
  
  void imap_close(int|void hard)
    {
      if (hard || !strlen(write_buffer))
	fd->close();
      else
	closing = 1;
    }

  void show_backtrace(mixed e)
    {
      werror(describe_backtrace(e));
    }
  
  void imap_read_callback(mixed id, string s)
    {
      add_data(s);

      string action;
      if (!current_request)
      {
	current_request = get_request();
    
	if (!current_request)
	  return;

	mixed e;
	if (e = catch(action = current_request->process(session, db, imap_send)))
	{
	  show_backtrace(e);
	  imap_send(current_request->tag, "BAD", "Internal error");
	  action = "finished";
	}
      }
      else
      {
	switch(current_request->expects)
	{
	case "literal":
	  string literal = buffer->get_literal(current_request->expected_length);
	  if (!literal)
	    return;

	  
	  mixed e;
	  if (e = catch(action = current_request
			->process_literal(session, db,
					  literal, imap_send)))
	  {
	    show_backtrace(e);
	    imap_send(current_request->tag, "BAD", "Internal error");
	    action = "finished";
	  }
	    
	  break;
	case "line":
	  string line = buffer->get_line();
	  if (!line)
	    return;
	    
	  mixed e;
	  if (e = catch(action = current_request
			->process_line(session, db, line, imap_send)))
	  {
	    show_backtrace(e);
	    imap_send(current_request->tag, "BAD", "Internal error");
	    action = "finished";
	  }

	  break;
	default:
	  throw( ({ "IMAP.pmod: Internal error\n", backtrace() }) );
	}
      }
      switch(action)
      {
      case "close":
	imap_close();
	break;
      case "progress":
	break;
      case "finished":
	current_request = 0;
	break;
      case "login":
	// uid = current_request->get_uid();
	state = "authenticated";
	current_request = 0;
	break;
      case "select":
	// mailbox = current_request->get_mailbox();
	state = "selected";
	current_request = 0;
	break;
      default:
	throw( ({ sprintf("IMAP.pmod: Internal error, action = %O\n",
			  action), backtrace() }) );
      }
    }
  
  void create(object f, object server, string|void state, int|void debug)
    {
      fd = f;
      db = server;
      
      ::create(state || "non_authenticated", debug);

      fd->set_nonblocking(imap_read_callback, 0,
			  die);
      imap_send("*", "OK", "IMAP4", "IMAP4rev1", "Service ready");
    }
}
#endif

object db;

int debug_level;

object port;

void accept_callback(mixed id)
{
  if (debug_level)
    werror("IMAP accept\n");

  object f = port->accept();
  if (f)
    connection(f, db, 0, debug_level);
}

void create(object p, int portnr, object server, int|void debug)
{
  port = p;
  db = server;
  debug_level = debug;

  if (!port->bind(portnr, accept_callback))
    throw( ({ "IMAP.imap_server->create: bind failed (port already bound?)\n",
              backtrace() }) );
  if (debug_level)
    werror("IMAP: Bound to port %d\n", portnr);
}

void close()
{
  if (port)
    destruct(port);
}
