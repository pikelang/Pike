/* sslfile.pike
 *
 */

inherit Stdio.File : socket;
inherit "connection" : connection;

object context;
  
string read_buffer; /* Data that is recieved before there is any
		     * read_callback */
string write_buffer; /* Data to be written */
function(mixed,string:void) read_callback;
function(mixed:void) write_callback;
function(mixed:void) close_callback;
function(mixed:void) accept_callback;

int connected; /* 1 if the connect callback has been called */
int blocking;  /* 1 if in blocking mode.
		* So far, there's no true blocking i/o, read and write
		* requests are just queued up. */

private void die(int status)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->die\n");
#endif
  if (status < 0)
  {
    /* Other end closed without sending a close_notify alert */
    context->purge_session(this_object());
#ifdef SSL3_DEBUG
    werror("SSL.sslfile: Killed\n");
#endif
  }
  if (close_callback)
    close_callback(socket::query_id());
  socket::close();
}
  
/* Return 0 if the connection is still alive,
   * 1 if it was closed politely, and -1 if it died unexpectedly
   */
private int try_write()
{
  int|string data = to_write();
#ifdef SSL3_DEBUG
  werror(sprintf("sslport->try_write: '%O'\n", data));
#endif
  if (stringp(data))
    write_buffer += data;
  if (strlen(write_buffer))
  {
    int written = socket::write(write_buffer);
    if (written > 0)
      write_buffer = write_buffer[written..];
    else
    {
#ifdef SSL3_DEBUG
      werror("SSL.sslfile->try_write: write failed\n");
#endif
      return -1;
    }
  }
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->try_write: end\n");
#endif
  return stringp(data) ? 0 : data;
}

void close()
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->close\n");
#endif
  send_packet(Alert(ALERT_warning, ALERT_close_notify));
  try_write();
  read_callback = 0;
  write_callback = 0;
  close_callback = 0;
}

int write(string s)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->write\n");
#endif
  int len = strlen(s);
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
  return len;
}

string read(mixed ...args)
{
  throw( ({ "SSL->sslfile: read() is not supported.\n", backtrace() }) );
}
  
private void ssl_read_callback(mixed id, string s)
{
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.sslfile->ssl_read_callback\n"));
#endif
  string|int data = got_data(s);
  if (stringp(data))
  {
#ifdef SSL3_DEBUG
    werror(sprintf("SSL.sslfile: application_data: '%s'\n", data));
#endif
    if (strlen(data))
    {
      read_buffer += data;
      if (!connected)
      {
	connected = 1;
	if (accept_callback)
	  accept_callback(this_object());
      }
      if (!blocking && read_callback && strlen(read_buffer))
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
  int res = try_write();
  if (res)
    die(res);
}
  
private void ssl_write_callback(mixed id)
{
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.sslport->ssl_write_callback: handshake_finished = %d\n"
		 "blocking = %d, write_callback = %O\n",
		 handshake_finished, blocking, write_callback));
#endif
  int res;
  
  if ( !(res = try_write()) && !strlen(write_buffer)
       && handshake_finished && !blocking && write_callback)
  {
#ifdef SSL3_DEBUG
  werror("SSL.sslport->ssl_write_callback: Calling write_callback\n");
#endif
    
    write_callback(id);
    res = try_write();
  }
  if (res)
    die(res);
}

private void ssl_close_callback(mixed id)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslport: ssl_close_callback\n");
#endif
  die(-1);
}

void set_accept_callback(function(mixed:void) a)
{
  accept_callback = a;
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
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->set_nonblocking\n");
#endif
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
  blocking = 0;
  if (strlen(read_buffer))
    ssl_read_callback(socket::query_id(), "");
  ssl_write_callback(socket::query_id());
}

void set_blocking()
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->set_blocking\n");
#endif
#if 0
  if (!connected)
    throw( ({ "SSL.sslfile->set_blocking: Not supported\n",
		backtrace() }) );
#endif
  blocking = 1;
}

#if 0
object accept()
{
  /* Dummy method, for compatibility with Stdio.Port */
  return this_object();
}
#endif

void create(object f, object c)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->create\n");
#endif
  context = c;
  read_buffer = write_buffer = "";
  socket::assign(f);
  socket::set_nonblocking(ssl_read_callback, ssl_write_callback, ssl_close_callback);
  connection::create(1);
}

