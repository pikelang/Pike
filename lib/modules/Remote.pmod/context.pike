
#include "remote.h"

object server_context;
object con;

string base;
int counter;

mapping id2val = ([ ]);
mapping val2id = ([ ]);
mapping other  = ([ ]);

string id_for(mixed thing)
{
  string id;
  if(id=val2id[thing])
    return id;

  if(server_context && (id=server_context->id_for(thing)))
    return id;
  
  val2id[thing] = id = (base+(counter++));
  id2val[id] = thing;
  return id;
}

object object_for(string id)
{
  object o;
  if(o=id2val[id])
    return o;
  if(o=other[id])
    return o;
  if(server_context && (o=server_context->object_for(id, con)))
    return other[id]=o;
  return other[id] = Obj(id, con, this_object());
}

object function_for(string id)
{
  object o;
  if(o=id2val[id])
    return o;
  if(o=other[id])
    return o;
  return other[id] = Call(0, id, con, this_object(), 0);
}

// Encoding:
//
//  error      -> ({ CTX_ERROR, data })
//  object     -> ({ CTX_OBJECT, id })
//  function   -> ({ CTX_FUNCTION, id })
//  program    -> ({ CTX_PROGRAM, id })
//  other      -> ({ CTX_OTHER, data })
//  call       -> ({ CTX_CALL_SYNC, ... })
//  array      -> ({ CTX_ARRAY, a })
//  return     -> ({ CTX_RETURN, id, data })
//  mapping    -> ({ CTX_MAPPING, m })
//

array encode(mixed val)
{
  if (objectp(val))
    return ({ CTX_OBJECT, id_for(val) });
  if (functionp(val) || programp(val))
    return ({ CTX_FUNCTION, id_for(val) });
  if (arrayp(val))
    return ({ CTX_ARRAY, Array.map(val, encode) });
  if (mappingp(val))
  {
    mapping m = ([ ]);
    foreach(indices(val), mixed i)
      m[i] = encode(val[i]);
    return ({ CTX_MAPPING, m });
  }
  return ({ CTX_OTHER, val });
}

array encode_error(string e)
{
  return ({ CTX_ERROR, e });
}

array encode_error_return(int id, string e)
{
  return ({ CTX_RETURN, id, encode_error(e) });
}

array encode_return(int id, mixed val)
{
  return ({ CTX_RETURN, id, encode(val) });
}

mixed decode(array a)
{
  // werror(sprintf("decode(%O)\n", a));
  switch(a[0]) {
  case CTX_ERROR:
    throw(({ "Remote error: "+a[1]+"\n", backtrace() }));
  case CTX_OTHER:
    return a[1];
  case CTX_OBJECT:
    return object_for(a[1]);
  case CTX_FUNCTION:
    return function_for(a[1]);
  case CTX_ARRAY:
    return Array.map(a[1], decode);
  case CTX_MAPPING:
    mapping m = ([ ]);
    foreach(indices(a[1]), mixed i)
      m[i] = decode(a[1][i]);
    return m;
  }
}

array encode_call(object|string o, string|function f, array args, int async)
{
  int type = (async ? CTX_CALL_ASYNC : CTX_CALL_SYNC);
  if(objectp(o))
    if(stringp(f))
      return ({ type, encode(o), f, encode(args), counter++ });
    else
      error("If the first argument is an object, the second must be a string");
  else if(stringp(o))
    if(stringp(f))
      return ({ type, ({ CTX_OBJECT, o }), f, encode(args), counter++ });
    else
      error("If the first argument is an object reference, "
	    "the second must be a string");
  else if(o)
    error("Error in arguments");
  else if(functionp(f)||programp(f))
    return ({ type, 0, encode(f), encode(args), counter++ });
  else if(stringp(f))
    return ({ type, 0, ({ CTX_FUNCTION, f }), encode(args), counter++ });
  error("Error in arguments");
}

function decode_call(array data)
{
  if((data[0] != CTX_CALL_SYNC) && (data[0] != CTX_CALL_ASYNC))
    error("This is not a call");
  if(data[1])
  {
    object o = decode(data[1]);
    return o[data[2]];
  }
  else
    return decode(data[2]);
}

void add(object o, string id)
{
  id2val[id] = o;
  val2id[o]  = id;
}

string describe(array data)
{
  switch(data[0]) {
  case CTX_ERROR:
    return "ERROR "+sprintf("%O",data[1]);
  case CTX_OTHER:
    if(stringp(data[1]))
      return "\""+data[1]+"\"";
    return (string)data[1];
  case CTX_OBJECT:
    return "<object "+data[1]+">";
  case CTX_FUNCTION:
    return "<function "+data[1]+">";
  case CTX_CALL_SYNC:
  case CTX_CALL_ASYNC:
    return (data[1] ? data[1][1]+"->"+data[2] : "<function "+data[2][1]+">") +
      "(" + Array.map(data[3][1], describe)*"," + ") ["+data[4]+" "+
      (data[0]==CTX_CALL_SYNC ? "sync" : "async" ) + "]";
  case CTX_RETURN:
    return "return["+data[1]+"]: " + describe(data[2]);
  case CTX_MAPPING:
    return "<mapping "+sizeof(indices(data[1]))+">";
  case CTX_ARRAY:
    return "<array "+sizeof(data[1])+">";
  }
  return "<unknown "+data[0]+">";
}

void set_server_context(object ctx, object cn)
{
  server_context = ctx;
  con = cn;
}

void create(string b, object|void cn)
{
  con = cn;
  base = b;
  counter = 0;
}
