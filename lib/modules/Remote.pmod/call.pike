
#include "remote.h"

string objectid;
string name;
object con;
object ctx;
int _async;

mixed `() (mixed ... args)
{
  mixed data = ctx->encode_call(objectid, name, args, _async);
  if (_async)
    con->call_async(data);
  else
    return con->call_sync(data);
  return 0;
}

mixed sync(mixed ... args)
{
  mixed data = ctx->encode_call(objectid, name, args, _async);
  return con->call_sync(data);
}

void async(mixed ... args)
{
  mixed data = ctx->encode_call(objectid, name, args, _async);
  con->call_async(data);
}

int is_async()
{
  return _async;
}

void set_async(int a)
{
  _async = a;
}

void create(string oid, string n, object cn, object ct, int a)
{
  objectid = oid;
  name = n;
  con = cn;
  ctx = ct;
  _async = a;
}
