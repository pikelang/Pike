#pike __REAL_VERSION__

#include "remote.h"

#ifdef REMOTE_DEBUG
private static int _debug_conn_nr;
#undef DEBUGMSG
#define DEBUGMSG(X) werror("<" + _debug_conn_nr + "> " + (X))
#endif

int closed, want_close, outstanding_calls, do_calls_async, max_call_threads;
object con;
object ctx;
array(function) close_callbacks = ({ });

int nice; // don't throw from call_sync

// - create

void create(void|int _nice, void|int _max_call_threads)
{
   nice=_nice;
   max_call_threads = _max_call_threads;
   if (!max_call_threads)
     call_out( lambda(){ do_calls_async=1; }, 0.1 ); // :-)
}

// - connect
//
// This function is called by clients to connect to a server.
//
int connect(string host, int port, void|int timeout)
{
#ifdef REMOTE_DEBUG
  _debug_conn_nr = all_constants()->Remote_debug_conn_nr;
  add_constant ("Remote_debug_conn_nr", _debug_conn_nr + 1);
#endif

  string s, sv;
  int end_time=time()+(timeout||60);

  if (closed)
    error("Can't reopen a closed connection");

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
#ifdef REMOTE_DEBUG
  _debug_conn_nr = all_constants()->Remote_debug_conn_nr;
  add_constant ("Remote_debug_conn_nr", _debug_conn_nr + 1);
#endif

  DEBUGMSG("starting server\n");
  if(con)
    error("Already connected to "+con->query_address());

  con = c;
  con->write("Pike remote server "+PROTO_VERSION+"\n");
  ctx = cx;

#if constant(thread_create)
  thread_create (server_read_thread);
#else
  con->set_nonblocking(handshake, write_some, closed_connection);
#endif
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
// Closes the connection nicely, after waiting in outstanding calls in
// both directions.
//
void close()
{
  want_close = 1;
#if constant(thread_create)
  calls->write(0);
#else
  try_close();
#endif
}

void destroy()
{
  catch (try_close());
}


void try_close()
{
  if (closed) return;

#if constant(thread_create)
  object rlock = read_cond_mutex->lock();
#endif

  if (!(sizeof (pending_calls) || outstanding_calls || sizeof (write_buffer))) {
#if constant(thread_create)
    // Nasty kludge: Send some nonsense to make the other end respond.
    // It's the only safe way to yank our read thread out of the
    // blocking read so that the socket _really_ closes.
    closed = -1;
    catch {
      // This should work with most older Remote implementations.
      string s = encode_value(ctx->encode_call (0, "sOmE nOnSeNsE..", ({}),
						CTX_CALL_SYNC));
      con->write (sprintf("%4c%s", sizeof(s), s));
    };
#endif
    if (!catch {con->set_blocking(); con->close();}) {
#if constant(thread_create)
      rlock = 0;
#endif
      catch (closed_connection());
    }
  }
  else
    DEBUGMSG("close delayed\n");
}

void closed_connection(int|void ignore)
{
  if (closed > 0) return;
  closed=1;
  DEBUGMSG("connection closed\n");

#if constant(thread_create)
  write_cond->broadcast();
  object lock = read_cond_mutex->lock();
  read_cond->broadcast();
  lock = 0;
  calls->write(0);
#endif

  foreach(close_callbacks, function|array f)
    if(functionp(f))
       f();
    else if (functionp(f[0]))
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
  if (want_close) {
    try_close();
    return 0;
  }
  return 1;
}

#if constant(thread_create)
Thread.Condition write_cond = Thread.Condition();
Thread.Condition read_cond  = Thread.Condition();
Thread.Mutex read_cond_mutex = Thread.Mutex();
Thread.Queue calls = Thread.Queue();
int call_threads;
#endif

void send(string s)
{
#if constant(thread_create)
  write_buffer += s;
  write_cond->signal();
#else
  string ob = write_buffer;
  write_buffer += s;
  if(!strlen(ob))
    write_some();
#endif
}

mapping(int:int) pending_calls = ([ ]);
mapping finished_calls = ([ ]);
mapping(int:string) errors = ([ ]);
string read_buffer = "";
int request_size = 0;

void provide_result(int refno, mixed result)
{
    finished_calls[ refno ] = result;
//   if (functionp(pending_calls[refno]))
//   {
//     DEBUGMSG("calling completion function for request "+refno+"\n");
//     pending_calls[refno]( result );
//   }
//   else
//   {
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

void do_call (array data)
{
  outstanding_calls++;
  int refno = data[4];
  object|function f = ctx->decode_call(data);
  array args = ctx->decode(data[3]);

  if (closed || want_close)
    return_error (refno, ({"Connection closed\n", ({backtrace()[-1]})}));
  else
    switch (data[0]) {
      case CTX_CALL_SYNC: // a synchronous call
	mixed res;
	mixed e = catch { res = f(@args); };
	if (e) {
	  catch (e[1] = e[1][sizeof(backtrace())-1..]);
	  return_error(refno, e);
	}
	else
	  return_value(refno, res);
	break;

      case CTX_CALL_ASYNC: // an asynchronous call
	e = catch { f(@args); };
	if (e) {
	  catch (e[1] = e[1][sizeof(backtrace())-1..]);
	  return_error(refno, e);
	}
    }
  outstanding_calls--;
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
#if !constant(thread_create)
      con->set_read_callback(read_some);
#endif
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
  
  if (request_size && sizeof(read_buffer) >= request_size && !closed)
  {
    array data = decode_value(read_buffer[0..request_size-1]);
    DEBUGMSG("got " + request_size + " bytes of message: "+ctx->describe(data)+"\n");
    read_buffer = read_buffer[request_size..];
    request_size = 0;
    switch(data[0]) {

     case CTX_ERROR:
       throw(({ "Remote error: "+data[1]+"\n", backtrace() }));
      
     case CTX_CALL_SYNC:
     case CTX_CALL_ASYNC:
#if constant(thread_create)
       if (max_call_threads) {
	 if (outstanding_calls >= call_threads && call_threads < max_call_threads)
	   call_threads++, thread_create(call_thread);
	 calls->write(data);
       }
       else {
	 if( do_calls_async )
	   call_out( do_call, 0, data);
	 else
	 {
	   if (!call_threads) call_threads++, thread_create(call_thread);
	   calls->write(data);
	 }
       }
#else
       do_call(data);
#endif
       break;

     case CTX_RETURN: // a returned value
       int refno = data[1];
       if (data[2][0] == CTX_ERROR) {
	 DEBUGMSG("providing error for request "+refno+"\n");
	 if (pending_calls[refno]) {
	   errors[refno] = data[2][1];
	   finished_calls[refno] = 17;
	 }
	 else
	   werror ("Remote async error: " + data[2][1] + "\n");
       }
       else {
	 mixed result = ctx->decode(data[2]);
	 DEBUGMSG("providing the result for request "+refno+"\n");
	 provide_result(refno, result);
       }
       break;

     case CTX_EXISTS:
       return_value (data[4], ctx->decode_existp (data));
       break;

     default:
       error("Unknown message");
    }
    if(sizeof(read_buffer) > 4) read_some(ignore,"");
  }
}

int read_once()
{
  if (closed) return 0;
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
  DEBUGMSG("write_thread\n");
  while( write_some() )
    if(!(strlen(write_buffer) || closed)) {
      if (want_close) try_close();
      write_cond->wait();
    }
  closed_connection();
  DEBUGMSG("write_thread exit\n");
}

void read_thread()
{
  DEBUGMSG("read_thread\n");
  con->set_blocking();
  thread_create( write_thread );
  while( read_once() ) {
    object lock = read_cond_mutex->lock();
    read_cond->broadcast();
    lock = 0;
  }
  closed_connection();
  DEBUGMSG("read_thread exit\n");
}

void server_read_thread()
{
  mixed err = catch {
    DEBUGMSG("server_read_thread\n");
    con->set_blocking();
    handshake (0, con->read (24));
    read_thread();
  };
  con = 0;
  DEBUGMSG("server_read_thread exit\n");
  if (err) throw (err);
}

void call_thread()
{
  DEBUGMSG("call_thread\n");
  while (!closed) {
    array data = calls->read();
    if (!data) try_close();
    else {
      if( do_calls_async )
	call_out( do_call, 0,data);
      else
	do_call( data );
    }
  }
  calls->write(0);
  DEBUGMSG("call_thread exit\n");
}
#endif

// - call_sync
//
// Make a call and wait for the result
//
mixed call_sync(array data)
{
  DEBUGMSG("call_sync "+ctx->describe(data)+"\n");
  if(closed || want_close) {
    error("Connection closed");
  }
  int refno = data[4];
//   werror("call_sync["+con->query_address()+"]["+refno+"] starting\n");
  string s = encode_value(data);
#if constant(thread_create)
  object lock = read_cond_mutex->lock();
#endif
  pending_calls[refno] = 1;
  mixed err = catch
  {
    send(sprintf("%4c%s", sizeof(s), s));
#if constant(thread_create)
    while(!closed && zero_type(finished_calls[refno]))
      read_cond->wait(lock);
#else
    con->set_blocking();
    while(!closed && zero_type(finished_calls[refno]))
      read_once();
#endif
    if (errors[refno]) {
      string e = errors[refno];
      m_delete (errors, refno);
      throw (({"Remote error: " + e + "\n", backtrace()}));
    }
  };
  m_delete (pending_calls, refno);
#if constant(thread_create)
  lock = 0;
#endif
  mixed err2 = catch {
#if !constant(thread_create)
    con->set_nonblocking(read_some, write_some, closed_connection);
#endif
    if (want_close) try_close();
  };
  if (err || err2)
  {
    catch(get_result(refno));
    throw (err || err2);
  }
//   werror("call_sync["+con->query_address()+"]["+refno+"] done\n");
  if (zero_type(finished_calls[refno])) {
    DEBUGMSG("connection closed in sync call " + refno + "\n");
    catch(get_result(refno));
    if (!nice)
      error("Could not read");
    else
      return UNDEFINED;
  }
  return get_result(refno);
}

// - call_async
//
// Make a call but don't wait for the result
//
void call_async(array data)
{
  DEBUGMSG("call_async "+ctx->describe(data)+"\n");
  if(closed || want_close)
    error("Connection closed");
  string s = encode_value(data);
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
