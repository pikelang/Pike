
#include "remote.h"

int closed;
object con;
object ctx;
array(function) close_callbacks = ({ });

void handshake(int ignore, string s);
void read_some(int ignore, string s);
void write_some(int|void ignore);
void closed_connection(int|void ignore);

// - connect
//
// This function is called by clients to connect to a server.
//
int connect(string host, int port)
{
  string s, sv;

  DEBUGMSG("connecting to "+host+":"+port+"...\n");

  if(con)
    error("Already connected to "+con->query_address());

  con = Stdio.File();
  if(!con->connect(host, port))
    return 0;
  DEBUGMSG("connected\n");
  con->write("Pike remote client "+PROTO_VERSION+"\n");
  s = con->read(24);
  if((sscanf(s,"Pike remote server %4s\n", sv) == 1) && (sv == PROTO_VERSION))
  {
    ctx = Context(replace(con->query_address(1), " ", "-"), this_object());
    con->set_nonblocking(read_some, write_some, closed_connection);
    return 1;
  }
  return 0;
}

// - start_server
//
// This function is called by servers when they have got a connection
// from a client. The first argument is the connection file object, and
// the second argument is the context to be used.
//
void start_server(object c, object cx)
{
  DEBUGMSG("starting server\n");
  if(con)
    error("Already connected to "+con->query_address());

  con = c;
  con->write("Pike remote server "+PROTO_VERSION+"\n");
  ctx = cx;

  con->set_nonblocking(handshake, write_some, closed_connection);
}

// - add_close_callback
//
// Add a function that is called when the connection is closed.
//
void add_close_callback(function f, mixed ... args)
{
  if(sizeof(args))
    close_callbacks += ({ ({f})+args });
  else
    close_callbacks += ({ f });
}

// - remove_close_callback
//
// Remove a function that is called when the connection is closed.
//
void remove_close_callback(array f)
{
  close_callbacks -= ({ f });
}


void closed_connection(int|void ignore)
{
  DEBUGMSG("connection closed\n");
  foreach(close_callbacks, function|array f)
    if(functionp(f))
       f();
    else
      f[0](@f[1..]);
  closed=1;
}

string write_buffer = "";
void write_some(int|void ignore)
{
  if(closed) {
    write_buffer="";
    return;
  }
  int c;
  if (!sizeof(write_buffer))
    return;
  c = con->write(write_buffer);
  if(c <= 0) return;
  write_buffer = write_buffer[c..];
  DEBUGMSG("wrote "+c+" bytes\n");
}

void send(string s)
{
  string ob = write_buffer;
  write_buffer += s;
  if(!strlen(ob)) write_some();
}

mapping pending_calls = ([ ]);
mapping finished_calls = ([ ]);
string read_buffer = "";
int request_size = 0;

void provide_result(int refno, mixed result)
{
  if (functionp(pending_calls[refno]))
  {
    DEBUGMSG("calling completion function for request "+refno+"\n");
    pending_calls[refno]( result );
  }
  else
  {
    finished_calls[refno] = result;
    m_delete(pending_calls, refno);
  }
}

mixed get_result(int refno)
{
  mixed r = finished_calls[refno];
  if (zero_type(r))
    error("Tried to get a result too early");
  m_delete(finished_calls, refno);
  return r;
}


void return_error(int refno, mixed err)
{
  string s = encode_value(ctx->encode_error_return(refno,
						   describe_backtrace(err)));
  send(sprintf("%4c%s", sizeof(s), s));
}

void return_value(int refno, mixed val)
{
  string s = encode_value(ctx->encode_return(refno, val));
  DEBUGMSG("return "+strlen(s)+" bytes ["+refno+"]\n");
  send(sprintf("%4c%s", sizeof(s), s));
}

void handshake(int ignore, string s)
{
  DEBUGMSG("handshake read "+sizeof(s)+" bytes\n");
  read_buffer += s;
  if (sizeof(read_buffer) >= 24)
  {
    string proto;
    if ((sscanf(read_buffer, "Pike remote client %4s\n", proto) == 1) &&
	(proto == PROTO_VERSION))
    {
      DEBUGMSG("handshake complete (proto="+proto+")\n");
      read_buffer = read_buffer[24..];
      con->set_read_callback(read_some);
      read_some(0,"");
    }
    else
      con->close();
  }
}

void read_some(int ignore, string s)
{
  if (!s) s = "";
  DEBUGMSG("read "+sizeof(s)+" bytes\n");
  read_buffer += s;
  DEBUGMSG("has "+sizeof(read_buffer)+" bytes\n");
  if(!strlen(read_buffer)) return;

  if (!request_size && sizeof(read_buffer) > 4)
  {
    sscanf(read_buffer, "%4c%s", request_size, read_buffer);
  }
  
  if (request_size && sizeof(read_buffer) >= request_size)
  {
    array data = decode_value(read_buffer[0..request_size-1]);
    read_buffer = read_buffer[request_size..];
    request_size = 0;
    DEBUGMSG("got message: "+ctx->describe(data)+"\n");
    switch(data[0]) {

     case CTX_ERROR:
       throw(({ "Remote error: "+data[1]+"\n", backtrace() }));
      
     case CTX_CALL_SYNC: // a synchrounous call
       int refno = data[4];
       object|function f = ctx->decode_call(data);
       array args = ctx->decode(data[3]);
       mixed res;
       mixed e = catch { res = f(@args); };
       if (e)
	 return_error(refno, e);
       else
	 return_value(refno, res);
       break;

     case CTX_CALL_ASYNC: // an asynchrounous call
       int refno = data[4];
       object|function f = ctx->decode_call(data);
       array args = ctx->decode(data[3]);
       mixed e = catch { f(@args); };
       if (e)
	 return_error(refno, e);
       break;

     case CTX_RETURN: // a returned value
       int refno = data[1];
       mixed result = ctx->decode(data[2]);
       if (!pending_calls[refno])
	 error("Got return for odd call: "+refno+"\n");
       DEBUGMSG("providing the result for request "+refno+": "+
		ctx->describe(data)+"\n");
       provide_result(refno, result);
       break;

     default:
       error("Unknown message");
    }
    if(sizeof(read_buffer) > 4) read_some(ignore,"");
  }
}


// - call_sync
//
// Make a call and wait for the result
//
mixed call_sync(array data)
{
  if(closed) {
    error("connection closed\n");
  }
  int refno = data[4];
  string s = encode_value(data);
  con->set_blocking();
  DEBUGMSG("call_sync "+ctx->describe(data)+"\n");
  pending_calls[refno] = 17; // a mutex lock key maybe?
  send(sprintf("%4c%s", sizeof(s), s));
  while(zero_type(finished_calls[refno]))
  {
    string s = con->read(8192,1);
    if(!s || !strlen(s))
    {
      closed_connection();
      error("Could not read");
    }
    read_some(0,s);
  }
  con->set_nonblocking(read_some, write_some, closed_connection);
  return get_result(refno);
}

// - call_async
//
// Make a call but don't wait for the result
//
void call_async(array data)
{
  if(closed) error("connection closed\n");
  string s = encode_value(data);
  DEBUGMSG("call_async "+ctx->describe(data)+"\n");
  send(sprintf("%4c%s", sizeof(s), s));
}

// - get_named_object
//
// Get a named object provided by the server.
//
object get_named_object(string name)
{
  DEBUGMSG("getting "+name+"\n");
  return ctx->object_for(name);
}
