
#include "remote.h"

int connected = 0;
object con;

object get(string name)
{
  if(!connected)
    error("Not connected");
  return con->get_named_object(name);
}


void create(string host, int port)
{
  con = Connection();
  if(!con->connect(host, port))
    error("Could not connect to server");
  connected = 1;
}

void provide(string name, mixed thing)
{
  con->ctx->add(name, thing);
}
