/* imap_server.pike
 *
 */

class connection
{
  inherit .server;

  object fd;

  object db; /* Mail backend */

  mixed uid;     /* Logged in user */
  mixed mailbox; /* Selected mailbox */

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
  
  void imap_send(string|object ...args)
    {
      /* FIXME: This code sends an \r\n-pair even if the last argument
       * is sent as a literal. */
      imap_write(Array.map(args, lambda(mixed x)
				   { return stringp(x) ? x : x->format(); } )
		 * " " + "\r\n");
      
      if (strlen(write_buffer))
	fd->set_write_callback(imap_write_callback);
    }

  void imap_send_line(string s)
    {
      imap_write(s + "\r\n");
    }
  
  void imap_close(int|void hard)
    {
      if (hard || !strlen(write_buffer))
	fd->close();
      else
	closing = 1;
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
    
	action = current_request->process(db, imap_send);
      }
      else
      {
	switch(current_request->expects)
	{
	case "literal":
	  string literal = buffer->get_literal(current_request->expected_length);
	  if (!literal)
	    return;
	  action = current_request->process_literal(db, literal, imap_send);
	  break;
	case "line":
	  string line = buffer->get_line();
	  if (!line)
	    return;
	  action = current_request->process_line(db, line, imap_send);
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
	uid = current_request->get_uid();
	state = "authenticated";
	current_request = 0;
	break;
      case "select":
	mailbox = current_request->get_mailbox();
	state = "selected";
	current_request = 0;
	break;
      default:
	throw( ({ sprintf("IMAP.pmod: Internal error, action = %O\n",
			  action), backtrace() }) );
      }
    }
  
  void create(object f, object server, string|void state)
    {
      fd = f;
      db = server;
      
      ::create(state || "non_authenticated");

      fd->set_nonblocking(imap_read_callback, 0,
			  die);
      imap_send("*", "OK", "IMAP4rev1", "Service ready");
    }
}

inherit Stdio.Port;
object db;

void accept_callback(mixed id)
{
  object f = accept();
  if (f)
    connection(f, db);
}

void create(int portnr, object server)
{
  db = server;
  if (!bind(portnr, accept_callback))
    throw( ({ "IMAP.imap_server->create: bind failed (port already bound?)\n",
	      backtrace() }) );
}
