#pike __REAL_VERSION__
#include "remote.h"
import ".";

int portno;
object port;
array connections = ({ });
object sctx;
int max_call_threads;

class Minicontext
{
  mapping(string:mixed) id2val = ([ ]);
  mapping(mixed:string) val2id = ([ ]);

  string id_for(mixed thing)
  {
    return val2id[thing];
  }

  object object_for(string id, object con)
  {
    object o = id2val[id];
    if(functionp(o) || programp(o))
      o = o(con);
    if(objectp(o) && functionp(o->close))
      con->add_close_callback(o->close);
    return o;
  }

  void add(string name, object|program what)
  {
    id2val[name] = what;
    val2id[what] = name;
  }
}

void got_connection(object f)
{
  object c = f->accept();
  c->set_blocking();
  object con = Connection(0, max_call_threads);
  object ctx = Context(gethostname()+"-"+portno);
  if (!c)
    error("accept failed\n");
  con->start_server(c, ctx);
  ctx->set_server_context(sctx, con);
  connections += ({ con });
}

void create(string host, int p, void|int _max_call_threads)
{
  portno = p;
  max_call_threads = _max_call_threads;
  port = Stdio.Port();
  port->set_id(port);
  if(host)
  {
    if(!port->bind(p, got_connection, host))
      error("Failed to bind to port\n");
  }
  else if(!port->bind(p, got_connection))
    error("Failed to bind to port\n");

  DEBUGMSG("listening to " + host + ":" + p + "\n");

  if(!portno)
    sscanf(port->query_address(), "%*s %d", portno);

  sctx = Minicontext();
}

void provide(string name, mixed thing)
{
  DEBUGMSG("providing "+name+"\n");
  sctx->add(name, thing);
}

void close()
{
  DEBUGMSG("closing listening port\n");
  destruct (port);
}

void close_all()
{
  DEBUGMSG("closing listening port and all connections\n");
  destruct (port);
  foreach (connections, object conn) conn->close();
}

int closed()
{
  return !!port;
}

void destroy()
{
  DEBUGMSG("destruct" + (port ? " - closing listening port\n" : "\n"));
  catch (destruct (port));
}
