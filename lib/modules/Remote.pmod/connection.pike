
#include "remote.h"

object con;
object ctx;

void handshake(int ignore, string s);
void read_some(int ignore, string s);
void write_some(int|void ignore);

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

  con->set_nonblocking(handshake, write_some, 0);
}


string write_buffer = "";
void write_some(int|void ignore)
{
  int c;
  if (!sizeof(write_buffer))
    return;
  c = con->write(write_buffer);
  write_buffer = write_buffer[c..];
  DEBUGMSG("wrote "+c+" bytes\n");
}

void send(string s)
{
  write_buffer += s;
  write_some();
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
	(proto = PROTO_VERSION))
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
  DEBUGMSG("read "+sizeof(s)+" bytes\n");
  read_buffer += s;

  if (!request_size && sizeof(read_buffer) > 4)
  {
    sscanf(read_buffer, "%4c%s", request_size, read_buffer);
    DEBUGMSG("reading message of size "+request_size+"\n");
  }

  if (request_size && sizeof(read_buffer) >= request_size)
  {
    array data = decode_value(read_buffer[0..request_size-1]);
    read_buffer = read_buffer[request_size..];
    request_size = 0;
    DEBUGMSG("got message: "+ctx->describe(data)+"\n");
    switch(data[0]) {

    case 0:
      throw(({ "Remote error: "+data[1]+"\n", backtrace() }));
      
    case 4: // a call
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

    case 6: // a returned value
      int refno = data[1];
      mixed result = ctx->decode(data[2]);
      if (!pending_calls[refno])
	error("Got an answer I didn't ask for (refno="+refno+")");
      DEBUGMSG("providing the result for request "+refno+": "+
	       ctx->describe(data)+"\n");
      provide_result(refno, result);
      break;

    default:
      error("Unknown message");
    }
  }
}


// - call_sync
//
// Make a call and wait for the result
//
mixed call_sync(array data)
{
  int refno = data[4];
  string s = encode_value(data);
  DEBUGMSG("call_sync "+ctx->describe(data)+"\n");
  pending_calls[refno] = 17; // a mutex lock key maybe?
  send(sprintf("%4c%s", sizeof(s), s));
  while(zero_type(finished_calls[refno]))
  {
    string s = con->read(8192,1);
    read_some(0,s);
  }
  return get_result(refno);
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
