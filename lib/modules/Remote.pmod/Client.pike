
#include "remote.h"

int connected = 0;
object con;
function close_callback = 0;

object get(string name)
{
  if(!connected)
    error("Not connected");
  return con->get_named_object(name);
}


void create(string host, int port, void|int nice, int ...timeout)
{
  con = Connection(nice);
  if(!con->connect(host, port, @timeout))
    error("Could not connect to server");
  connected = 1;
  con->closed = 0;
}

void provide(string name, mixed thing)
{
  con->ctx->add(name, thing);
}

void set_close_callback(function f)
{
  if(close_callback)
    con->remove_close_callback(close_callback);
  close_callback = f;
  con->add_close_callback(f);
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
