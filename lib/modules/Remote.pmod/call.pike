#pike __REAL_VERSION__


#include "remote.h"

constant is_remote_call = 1;

string objectid;
string name;
object con;
object ctx;
int _async;

mixed `() (mixed ... args)
{
  mixed data = ctx->encode_call(objectid, name, args,
				_async ? CTX_CALL_ASYNC : CTX_CALL_SYNC);
  if (_async)
    con->call_async(data);
  else
    return con->call_sync(data);
  return 0;
}

mixed sync(mixed ... args)
{
  mixed data = ctx->encode_call(objectid, name, args, CTX_CALL_SYNC);
  return con->call_sync(data);
}

void async(mixed ... args)
{
  mixed data = ctx->encode_call(objectid, name, args, CTX_CALL_ASYNC);
  con->call_async(data);
}

int exists()
{
  mixed data = ctx->encode_call(objectid, name, ({}), CTX_EXISTS);
  return con->call_sync(data);
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
