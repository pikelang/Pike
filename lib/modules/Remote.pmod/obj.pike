
#include "remote.h"

constant is_remote_obj = 1;

string id;
object con;
object ctx;

mapping calls = ([ ]);

mixed get_function(string f)
{
  object call = calls[f];
  if (!call)
    call = calls[f] = Call(id, f, con, ctx, 0);
  return call;
}

mixed `[] (string f)
{
  return get_function(f);
}

mixed `-> (string f)
{
  return get_function(f);
}

void create(string i, object cn, object ct)
{
  id  = i;
  con = cn;
  ctx = ct;
}
