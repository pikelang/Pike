#pike __REAL_VERSION__

/* sslport.pike
 *
 */

//! Interface similar to Stdio.Port.

inherit Stdio.Port : socket;
inherit "context";
inherit ADT.Queue : accept_queue;

constant sslfile = SSL.sslfile;

function(object:void) accept_callback;

void finished_callback(object f)
{
  accept_queue::put(f);
  while (accept_callback && !accept_queue::is_empty())
    accept_callback(query_id());
}

void ssl_callback(mixed id)
{
  object f = id->socket_accept();
  if (f)
  {
    sslfile(f, this)->set_accept_callback(finished_callback);
  }
}

#if 0
void set_id(mixed id)
{
  error( "Not supported\n" );
}

mixed query_id()
{
  error( "Not supported\n" );
}
#endif

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

object accept()
{
  return accept_queue::get();
}

void create()
{
#ifdef SSL3_DEBUG
  werror("SSL.sslport->create\n");
#endif
  context::create();
  accept_queue::create();
  set_id(this);
}
