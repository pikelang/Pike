
#include "remote.h"

string objectid;
string name;
object con;
object ctx;
int is_async;

mixed `() (mixed ... args)
{
  mixed data = ctx->encode_call(objectid, name, args, is_async);
  if (is_async)
    con->call_async(data);
  else
    return con->call_sync(data);
  return 0;
}

mixed sync(mixed ... args)
{
  mixed data = ctx->encode_call(objectid, name, args, is_async);
  return con->call_sync(data);
}

void async(mixed ... args)
{
  mixed data = ctx->encode_call(objectid, name, args, is_async);
  con->call_async(data);
}

void create(string oid, string n, object cn, object ct, int a)
{
  objectid = oid;
  name = n;
  con = cn;
  ctx = ct;
  is_async = a;
}
