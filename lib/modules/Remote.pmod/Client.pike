#pike __REAL_VERSION__
#include "remote.h"
import ".";

int connected = 0;
object con;
function close_callback = 0;

object get(string name)
{
  if(!connected)
    error("Not connected\n");
  return con->get_named_object(name);
}

// This indirection function also serves the purpose of keeping a ref
// to this object in case the user doesn't.
void client_close_callback()
{
  if (close_callback) close_callback();
}

void create(string host, int port, void|int nice,
	    void|int timeout, void|int max_call_threads)
{
  con = Connection(nice, max_call_threads);
  if(!con->connect(host, port, timeout))
    error("Could not connect to server.\n");
  connected = 1;
  con->closed = 0;
  con->add_close_callback (client_close_callback);
}

void provide(string name, mixed thing)
{
  con->ctx->add(thing, name);
}

void set_close_callback(function f)
{
  close_callback = f;
}

void close()
{
  con->close();
}

int closed()
{
  return con->closed;
}

void destroy()
{
  catch (con->close());
}
