
#include "remote.h"

int closed, want_close, reading;
object con;
object ctx;
array(function) close_callbacks = ({ });

int nice; // don't throw from call_sync

// - create

void create(void|int _nice)
{
   nice=_nice;
}

// - connect
//
// This function is called by clients to connect to a server.
//
int connect(string host, int port, int ... timeout)
{
  string s, sv;
  int end_time=time()+(sizeof(timeout)?(timeout[0]||1):60);

  DEBUGMSG("connecting to "+host+":"+port+"...\n");

  if(con)
    error("Already connected to "+con->query_address());

  con = Stdio.File();
  if(!con->connect(host, port))
    return 0;
  con->write("Pike remote client "+PROTO_VERSION+"\n");
  s="";
  con->set_nonblocking();
  for (;;)
  {
     s += (con->read(24-strlen(s),1)||"");
     if (strlen(s)==24) break;
     sleep(0.02);
     if (time()>end_time) 
     {
	con->close();
	return 0;
     }
  }
  if((sscanf(s,"Pike remote server %4s\n", sv) == 1) && 
     (sv == PROTO_VERSION))
  {
    DEBUGMSG("connected\n");
    ctx = Context(replace(con->query_address(1), " ", "-"), this_object());
#if constant(thread_create)
    thread_create( read_thread );
#else
    con->set_nonblocking(read_some, write_some, closed_connection);
#endif
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

// - close
//
// Closes the connection.
//
void close()
{
  want_close = 1;
  if (!(reading || sizeof (write_buffer))) {
    if (!catch {con->set_blocking(); con->close();})
      closed_connection();
  }
  else
    DEBUGMSG("close delayed\n");
}

void destroy()
{
  catch (con->close());
}


void closed_connection(int|void ignore)
{
  if (closed) return;
  closed=1;
  DEBUGMSG("connection closed\n");
  foreach(close_callbacks, function|array f)
    if(functionp(f))
       f();
    else
      f[0](@f[1..]);
}

string write_buffer = "";
int write_some(int|void ignore)
{
  if(closed) {
    write_buffer="";
    return 0;
  }
  if (sizeof (write_buffer)) {
    int c;
    c = con->write(write_buffer);
    if(c <= 0) return 0;
    write_buffer = write_buffer[c..];
    DEBUGMSG("wrote "+c+" bytes\n");
  }
  if (want_close && !(reading || sizeof(write_buffer)))
    if (!catch {con->set_blocking(); con->close();})
    {
      closed_connection();
      return 0;
    }
  return 1;
}

#if constant( Thread.Condition )
Thread.Condition write_cond = Thread.Condition();
Thread.Condition read_cond  = Thread.Condition();
#endif

void send(string s)
{
#if constant( Thread.Condition )
  string ob = write_buffer;
#endif
  write_buffer += s;
#if constant( Thread.Condition )
  write_cond->signal();
#else
  if(!strlen(ob)) 
    write_some();
#endif
}

// mapping pending_calls = ([ ]);
mapping finished_calls = ([ ]);
string read_buffer = "";
int request_size = 0;

void provide_result(int refno, mixed result)
{
//   if (functionp(pending_calls[refno]))
//   {
//     DEBUGMSG("calling completion function for request "+refno+"\n");
//     pending_calls[refno]( result );
//   }
//   else
//   {
    finished_calls[refno] = result;
//     m_delete(pending_calls, refno);
//   }
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
  
  if (request_size && sizeof(read_buffer) >= request_size && !want_close)
  {
    array data = decode_value(read_buffer[0..request_size-1]);
    DEBUGMSG("got " + request_size + " bytes of message: "+ctx->describe(data)+"\n");
    read_buffer = read_buffer[request_size..];
    request_size = 0;
    reading = 1;
    switch(data[0]) {

     case CTX_ERROR:
       throw(({ "Remote error: "+data[1]+"\n", backtrace() }));
      
     case CTX_CALL_SYNC: // a synchronous call
#if constant(thread_create)
       call_out( lambda(){
#endif
       int refno = data[4];
       object|function f = ctx->decode_call(data);
       array args = ctx->decode(data[3]);
       mixed res;
       mixed e = catch { res = f(@args); };
       if (e) {
	 catch (e[1] = e[1][sizeof(backtrace())..]);
	 return_error(refno, e);
       }
       else
	 return_value(refno, res);
#if constant(thread_create)
                 }, 0 );
#endif
       break;

     case CTX_CALL_ASYNC: // an asynchronous call
//        werror("async\n");
#if constant(thread_create)
       call_out( lambda(){
#endif
       int refno = data[4];
       object|function f = ctx->decode_call(data);
       array args = ctx->decode(data[3]);
       mixed e = catch { f(@args); };
       if (e) {
	 catch (e[1] = e[1][sizeof(backtrace())..]);
	 return_error(refno, e);
       }
#if constant(thread_create)
                 }, 0 );
#endif
       break;

     case CTX_RETURN: // a returned value
       int refno = data[1];
       mixed result = ctx->decode(data[2]);
       DEBUGMSG("providing the result for request "+refno+": "+
		ctx->describe(data)+"\n");
       provide_result(refno, result);
       break;

     default:
       error("Unknown message");
    }
    reading = 0;
    if(sizeof(read_buffer) > 4) read_some(ignore,"");
  }
}

#if constant(Thread.Mutex)
object(Thread.Mutex) block_read_mutex = Thread.Mutex();
#endif


// - call_sync
//
// Make a call and wait for the result
//

object reading_thread;

int read_once()
{
  string s = con->read( 8192, 1 );
  if( !s || !strlen(s) )
  {
    closed_connection( 0 );
    return 0;
  }
  read_some( 0, s );
  return 1;
}

#if constant(thread_create)
void write_thread()
{
  while( write_some() )
    if(!strlen(write_buffer))
      write_cond->wait();
  closed_connection();
}

void read_thread()
{
  con->set_blocking();
  thread_create( write_thread );
  while( read_once() )
    read_cond->broadcast();
  closed_connection();
}
#endif

int block; 
mixed call_sync(array data)
{
  if(closed) {
    error("connection closed\n");
  }
  int refno = data[4];
//   werror("call_sync["+con->query_address()+"]["+refno+"] starting\n");
  string s = encode_value(data);
  mixed err = catch 
  {
    DEBUGMSG("call_sync "+ctx->describe(data)+"\n");
//     pending_calls[refno] = 17; // a mutex lock key maybe?
    send(sprintf("%4c%s", sizeof(s), s));
#if constant(thread_create)
    while(zero_type(finished_calls[refno]))
      read_cond->wait();
#else
    con->set_blocking();
    while(zero_type(finished_calls[refno]))
      read_once();
#endif
  };
  mixed err2;
#if !constant(thread_create)
  err2 = catch {
    con->set_nonblocking(read_some, write_some, closed_connection);
  };
#endif
  if (err || err2) 
  {
    catch(get_result(refno));
    throw (err || err2);
  }
//   werror("call_sync["+con->query_address()+"]["+refno+"] done\n");
  return get_result(refno);
}

// - call_async
//
// Make a call but don't wait for the result
//
void call_async(array data)
{
  if(closed) 
    error("connection closed\n");
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
