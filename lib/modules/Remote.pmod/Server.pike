#pike __REAL_VERSION__
#include "remote.h"
import ".";

//! Remote RPC server.

int portno;

//! @decl Stdio.Port port
//!
//! Port for the @[Remote.Server].
object port;

//! @decl array(Connection) connections
//!
//! Open connections.
array connections = ({ });

//! @decl Minicontext sctx
//!
//! Server context.
object sctx;

int max_call_threads;

//! The server @[Context] class.
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
    error("Failed to accept connection: %s\n", strerror (f->errno()));
  con->start_server(c, ctx);
  ctx->set_server_context(sctx, con);
  connections += ({ con });
}

//! @decl void create(string host, int port, void|int max_call_threads)
//! Create a @[Remote.Server].
//!
//! @param host
//! @param port
//!   Hostname and port for the @[Remote.Server].
//!
//! @param max_call_threads
//!   Maximum number of concurrent threads.
void create(string host, int p, void|int _max_call_threads)
{
  portno = p;
  max_call_threads = _max_call_threads;
  port = Stdio.Port();
  port->set_id(port);
  if(host)
  {
    if(!port->bind(p, got_connection, host))
      error("Failed to bind port: %s\n", strerror (port->errno()));
  }
  else if(!port->bind(p, got_connection))
    error("Failed to bind port: %s\n", strerror (port->errno()));

  DEBUGMSG("listening to " + host + ":" + p + "\n");

  if(!portno)
    sscanf(port->query_address(), "%*s %d", portno);

  sctx = Minicontext();
}

//! Provide a named @[thing] to the @[Remote.Client](s).
//!
//! @param name
//!   Name to provide @[thing] under.
//!
//! @param thing
//!   Thing to provide.
void provide(string name, mixed thing)
{
  DEBUGMSG("providing "+name+"\n");
  sctx->add(name, thing);
}

//! Shut down the @[Remote.Server] for new connections.
void close()
{
  DEBUGMSG("closing listening port\n");
  destruct (port);
}

//! Shut down the @[Remote.Server] and terminate all current clients.
void close_all()
{
  DEBUGMSG("closing listening port and all connections\n");
  destruct (port);
  foreach (connections, object conn) conn->close();
}

//! Check if the @[Remote.Server] is closed.
int closed()
{
  return !!port;
}

void destroy()
{
  DEBUGMSG("destruct" + (port ? " - closing listening port\n" : "\n"));
  catch (destruct (port));
}
