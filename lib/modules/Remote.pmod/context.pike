
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
  return other[id] = Call(0, id, con, this_object());
}

// Encoding:
//
//  error      -> ({ 0, data })
//  object     -> ({ 2, id })
//  function   -> ({ 3, id })
//  program    -> ({ 3, id })
//  other      -> ({ 1, data })
//  call       -> ({ 4, ... })
//  array      -> ({ 5, a })
//  return     -> ({ 6, id, data })
//  mapping    -> ({ 7, m })
//

array encode(mixed val)
{
  if (objectp(val))
    return ({ 2, id_for(val) });
  if (functionp(val) || programp(val))
    return ({ 3, id_for(val) });
  if (arrayp(val))
    return ({ 5, Array.map(val, encode) });
  if (mappingp(val))
  {
    mapping m = ([ ]);
    foreach(indices(val), mixed i)
      m[i] = encode(val[i]);
    return ({ 7, m });
  }
  return ({ 1, val });
}

array encode_error(string e)
{
  return ({ 0, e });
}

array encode_error_return(int id, string e)
{
  return ({ 6, id, encode_error(e) });
}

array encode_return(int id, mixed val)
{
  return ({ 6, id, encode(val) });
}

mixed decode(array a)
{
  // werror(sprintf("decode(%O)\n", a));
  switch(a[0]) {
  case 0:
    throw(({ "Remote error: "+a[1]+"\n", backtrace() }));
  case 1:
    return a[1];
  case 2:
    return object_for(a[1]);
  case 3:
    return function_for(a[1]);
  case 5:
    return Array.map(a[1], decode);
  case 7:
    mapping m = ([ ]);
    foreach(indices(a[1]), mixed i)
      m[i] = decode(a[1][i]);
    return m;
  }
}

array encode_call(object|string o, string|function f, array args)
{
  if(objectp(o))
    if(stringp(f))
      return ({ 4, encode(o), f, encode(args), counter++ });
    else
      error("If the first argument is an object, the second must be a string");
  else if(stringp(o))
    if(stringp(f))
      return ({ 4, ({ 2, o }), f, encode(args), counter++ });
    else
      error("If the first argument is an object reference, "
	    "the second must be a string");
  else if(o)
    error("Error in arguments");
  else if(functionp(f)||programp(f))
    return ({ 4, 0, encode(f), encode(args), counter++ });
  else if(stringp(f))
    return ({ 4, 0, ({ 3, f }), encode(args), counter++ });
  error("Error in arguments");
}

function decode_call(array data)
{
  if(data[0]!=4)
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
  case 0:
    return "ERROR "+sprintf("%O",data[1]);
  case 1:
    if(stringp(data[1]))
      return "\""+data[1]+"\"";
    return (string)data[1];
  case 2:
    return "<object "+data[1]+">";
  case 3:
    return "<function "+data[1]+">";
  case 4:
    return (data[1] ? data[1][1]+"->"+data[2] : "<function "+data[2][1]+">") +
      "(" + Array.map(data[3][1], describe)*"," + ") ["+data[4]+"]";
  case 6:
    return "return["+data[1]+"]: " + describe(data[2]);
  case 7:
    return "<mapping "+sizeof(indices(data[1]))+">";
  }
  return "<unknown>";
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
