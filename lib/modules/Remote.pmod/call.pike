
#include "remote.h"

string objectid;
string name;
object con;
object ctx;

mixed `() (mixed ... args)
{
  return con->call_sync(ctx->encode_call(objectid, name, args));
}

void create(string oid, string n, object cn, object ct)
{
  objectid = oid;
  name = n;
  con = cn;
  ctx = ct;
}
