/* sslport.pike
 *
 */

inherit Stdio.Port : socket;
inherit "context";

function(object:void) accept_callback;

class sslfile
{
  inherit Stdio.File : socket;
  inherit "connection" : connection;

  object context;
  
  string read_buffer; /* Data that is recieved before there is any
		       * read_callback */
  string write_buffer; /* Data to be written */
  function(mixed,string:void) read_callback;
  function(mixed:void) write_callback;
  function(mixed:void) close_callback;

  int connected; /* 1 if the connect callback has been called */

  private void die(int status)
  {
    if (status < 0)
    {
      /* Other end closed without sending a close_notify alert */
      context->purge_session(this_object());
      werror("SSL.sslfile: Killed\n");
    }
    if (close_callback)
      close_callback(socket::query_id());
  }
  
  /* Return 0 if the connection is still alive,
   * 1 if it was closed politely, and -1 if it died unexpectedly
   */
  private int try_write()
  {
    int|string data = to_write();
//    werror(sprintf("sslport->try_write: %O\n", data));
    if (stringp(data))
    {
      write_buffer += data;
      if (strlen(write_buffer))
      {
	int written = socket::write(write_buffer);
	if (written > 0)
	  write_buffer = write_buffer[written..];
	else
	  werror("SSL.sslfile->try_write: write failed\n");
      }
      return 0;
    }
    socket::close();
    return data;
  }

  void close()
  {
    send_packet(Alert(ALERT_warning, ALERT_close_notify));
    try_write();
    read_callback = 0;
    write_callback = 0;
    close_callback = 0;
  }

  int write(string s)
  {
    object packet;
    int res;
    while(strlen(s))
    {
      packet = Packet();
      packet->content_type = PACKET_application_data;
      packet->fragment = s[..PACKET_MAX_SIZE-1];
      send_packet(packet);
      s = s[PACKET_MAX_SIZE..];
    }
    (res = try_write()) && die(res);
  }

  string read(mixed ...args)
  {
    throw( ({ "SSL->sslfile: read() is not supported.\n", backtrace() }) );
  }
  
  private void ssl_read_callback(mixed id, string s)
  {
    werror(sprintf("sslfile->ssl_read_callback\n"));
    string|int data = got_data(s);
    if (stringp(data))
    {
      werror(sprintf("sslfile: application_data: '%s'\n", data));
      if (strlen(data))
      {
	read_buffer += data;
	if (!connected)
	{
	  connected = 1;
	  context->accept_callback(this_object());
	}
	if (read_callback && strlen(read_buffer))
	{
	  read_callback(id, read_buffer + data);
	  read_buffer = "";
	}
      }
    }
    else
    {
      if (data < 0)
	/* Fatal error, remove from session cache */
	context->purge_session(this_object());
    }
    try_write() || (close_callback && close_callback());
  }
  
  private void ssl_write_callback(mixed id)
  {
    werror("SSL.sslport: ssl_write_callback\n");
    int res;
    if ( !(res = try_write()) && !strlen(write_buffer)
	 && handshake_finished && write_callback)
    {
      write_callback(id);
      res = try_write();
    }
    if (res)
      die(res);
  }

  private void ssl_close_callback(mixed id)
  {
    werror("SSL.sslport: ssl_close_callback\n");
    socket::close();
    die(-1);
  }
  
  void set_read_callback(function(mixed,string:void) r)
  {
    read_callback = r;
  }

  void set_write_callback(function(mixed:void) w)
  {
    write_callback = w;
  }

  void set_close_callback(function(mixed:void) c)
  {
    close_callback = c;
  }
  
  void set_nonblocking(function ...args)
  {
    switch (sizeof(args))
    {
    case 0:
      break;
    case 3:
      set_read_callback(args[0]);
      set_write_callback(args[1]);
      set_close_callback(args[2]);
      break;
    default:
      throw( ({ "SSL.sslfile->set_blocking: Wrong number of arguments\n",
		  backtrace() }) );
    }
  }

  void set_blocking()
  {
    throw( ({ "SSL.sslfile->set_blocking: Not supported\n",
		backtrace() }) );
  }

  object accept()
  {
    /* Dummy method, for compatibility with Stdio.Port */
    return this_object();
  }
  
  void create(object f, object c)
  {
    context = c;
    read_buffer = write_buffer = "";
    socket::assign(f);
    socket::set_nonblocking(ssl_read_callback, ssl_write_callback, ssl_close_callback);
    connection::create(1);
  }
}

void ssl_callback(mixed id)
{
  object f = id->socket_accept();
  if (f)
    sslfile(f, this_object());
}

void set_id(mixed id)
{
  throw( ({ "SSL.sslport->set_id: Not supported\n", backtrace() }) );
}

mixed query_id()
{
  throw( ({ "SSL.sslport->query_id: Not supported\n", backtrace() }) );
}

int bind(int port, function callback, string|void ip)
{
  accept_callback = callback;
  return socket::bind(port, ssl_callback, ip);
}

int listen_fd(int fd, function callback)
{
  accept_callback = callback;
  return socket::listen_fd(fd, callback);
}

object socket_accept()
{
  return socket::accept();
}

int accept()
{
  throw( ({ "SSL.sslport->accept: Not supported\n", backtrace() }) );
}
