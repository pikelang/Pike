#pike __REAL_VERSION__


#include "remote.h"

object server_context;
object con;

string base;
int counter;

mapping id2val = ([ ]);
mapping val2id = ([ ]);
mapping other  = ([ ]);
mapping val2other = ([ ]);

string id_for(mixed thing)
{
  string id;
  if(id=val2id[thing]) {
    DEBUGMSG(sprintf("id_for(%O) found locally: %s\n", thing, id));
    return id;
  }

  if(id=val2other[thing]) {
    DEBUGMSG(sprintf("id_for(%O) found remote: %s\n", thing, id));
    return id;
  }

  if(server_context && (id=server_context->id_for(thing))) {
    DEBUGMSG(sprintf("id_for(%O) found in server_context: %s\n", thing, id));
    return id;
  }
  
  val2id[thing] = id = (base+(counter++));
  id2val[id] = thing;
  DEBUGMSG(sprintf("id_for(%O) not found; added %s locally\n", thing, id));
  return id;
}

object object_for(string id)
{
  object o = id2val[id];
  int destructed = zero_type (id2val[id]) != 1;
  if(o) {
    DEBUGMSG("object_for(" + id + ") found locally\n");
    return o;
  }
  if(o=other[id]) {
    DEBUGMSG("object_for(" + id + ") found remote\n");
    return o;
  }
  if(server_context && (o=server_context->object_for(id, con))) {
    DEBUGMSG("object_for(" + id + ") found in server_context\n");
    val2id[o]=id;
    return id2val[id]=o;
  }
  if(destructed) {
    DEBUGMSG("object_for(" + id + ") found destructed locally\n");
    return 0;
  }
  DEBUGMSG("object_for(" + id + ") not found; making remote object\n");
  o = Obj(id, con, this_object());
  val2other[o] = id;
  return other[id] = o;
}

object function_for(string id)
{
  object o = id2val[id];
  if(zero_type (id2val[id]) != 1) {
    DEBUGMSG("function_for(" + id + ") found " + (o ? "" : "destructed ") + "locally\n");
    return o;
  }
  if(o=other[id]) {
    DEBUGMSG("function_for(" + id + ") found remote\n");
    return o;
  }
  DEBUGMSG("function_for(" + id + ") not found; making remote object\n");
  o = Call(0, id, con, this_object(), 0);
  val2other[o] = id;
  return other[id] = o;
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
  if (intp(val) || floatp(val))
    return ({ CTX_OTHER, val });
  if (objectp(val) && (object_program(val)->is_remote_obj || !val->_encode_object))
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
  if (e[-1] == '\n') e = e[..sizeof(e)-2];
  return ({ CTX_ERROR, gethostname()+":"+replace(e, "\n", "\n"+gethostname()+":") });
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
    error( "Remote error: "+a[1]+"\n" );
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

array encode_call(object|string o, string|function f, array args, int type)
{
  if(objectp(o))
    if(stringp(f) || !f)
      return ({ type, encode(o), f, encode(args), counter++ });
    else
      error("If the first argument is an object, "
	    "the second must be a string or zero.\n");
  else if(stringp(o))
    if(stringp(f) || !f)
      return ({ type, ({ CTX_OBJECT, o }), f, encode(args), counter++ });
    else
      error("If the first argument is an object reference, "
	    "the second must be a string or zero.\n");
  else if(o)
    error("Error in arguments.\n");
  else if(functionp(f)||programp(f))
    return ({ type, 0, encode(f), encode(args), counter++ });
  else if(stringp(f))
    return ({ type, 0, ({ CTX_FUNCTION, f }), encode(args), counter++ });
  error("Error in arguments.\n");
}

function|object decode_call(array data)
{
  if((data[0] != CTX_CALL_SYNC) && (data[0] != CTX_CALL_ASYNC))
    error("This is not a call.\n");
  if(data[1])
  {
    string id = data[1][1];
    object o = id2val[id];
    if (o)
      DEBUGMSG(id + " found locally\n");
    else if(!o && server_context && (o=server_context->object_for(id, con))) {
      DEBUGMSG(id + " found in server_context\n");
      val2id[o] = id;
      id2val[id] = o;
    }
#ifdef REMOTE_DEBUG
    if (!o) DEBUGMSG(id + " not found\n");
#endif
    if (data[2])
      return o && o[data[2]];
    else
      return o;
  }
  else {
    string id = data[2][1];
    object o = id2val[id];
#ifdef REMOTE_DEBUG
    if (!o) DEBUGMSG(id + " not found\n");
#endif
    return o;
  }
}

int decode_existp(array data)
{
  if(data[0] != CTX_EXISTS)
    error("This is not an exists check.\n");
  if(data[1])
  {
    string id = data[1][1];
    object o = id2val[id];
    if (o)
      DEBUGMSG(id + " found locally\n");
    else if(!o && server_context && (o=server_context->object_for(id, con))) {
      DEBUGMSG(id + " found in server_context\n");
      val2id[o] = id;
      id2val[id] = o;
    }
#ifdef REMOTE_DEBUG
    if (!o) DEBUGMSG(id + " not found\n");
#endif
    return !!o;
  }
  else {
    string id = data[2][1];
    object o = id2val[id];
#ifdef REMOTE_DEBUG
    if (!o) DEBUGMSG(id + " not found\n");
#endif
    return !!o;
  }
}

void add(object o, string id)
{
  DEBUGMSG(id + " added locally\n");
  id2val[id] = o;
  val2id[o]  = id;
}

string describe(array data)
{
  switch(data[0]) {
  case CTX_ERROR:
    return "ERROR "+sprintf("%O",data[1]);
  case CTX_OTHER:
    if (!stringp (data[1]) || sizeof (data[1]) < 64)
      return sprintf("%O",data[1]);
    else
      return sprintf("%O..(%db more)",data[1][..63], sizeof (data[1]) - 64);
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
  case CTX_EXISTS:
    return "exists["+(data[1] ? data[1][1]+"->"+data[2] :
		      "<function "+data[2][1]+">")+"]";
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
