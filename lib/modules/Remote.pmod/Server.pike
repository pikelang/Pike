
#include "remote.h"

int portno;
object port;
array connections = ({ });
object sctx;

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
    if(!o || objectp(o))
      return o;
    return o(con);
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
  object con = Connection();
  object ctx = Context(gethostname()+"-"+portno);
  if (!c)
    error("accept failed");
  con->start_server(c, ctx);
  ctx->set_server_context(sctx, con);
  connections += ({ con });
}

int create(string host, int p)
{
  portno = p;
  port = Stdio.Port();
  if(host)
  {
    if(!port->bind(p, got_connection, host))
      return 0;
  }
  else if(!port->bind(p, got_connection, host))
    return 0;

  sctx = Minicontext();
  return 1;
}

void provide(string name, mixed thing)
{
  DEBUGMSG("providing "+name+"\n");
  sctx->add(name, thing);
}
