#pike __REAL_VERSION__

/* $Id: sslfile.pike,v 1.1 2004/07/04 14:21:16 mast Exp $
 *
 */

//! Interface similar to Stdio.File.

inherit "cipher";
inherit SSL.connection : connection;

#ifdef SSL3_DEBUG_TRANSPORT
#define SSL3_DEBUG
#endif /* SSL3_DEBUG_TRANSPORT */

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG werror
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG
#endif /* SSL3_DEBUG */
object(Stdio.File) socket;

int _fd;

#ifdef THREAD_DEBUG

static Thread.Thread cur_thread;

static class MyLock
{
  Thread.Thread old_cur_thread;
  static void create (Thread.Thread t) {old_cur_thread = t;}
  static void destroy() {cur_thread = old_cur_thread;}
};

#define THREAD_CHECK							\
  MyLock lock;								\
  {									\
    Thread.Thread this_thr = this_thread();				\
    Thread.Thread old_cur_thread = cur_thread;				\
    /* Relying on interpreter lock here. */				\
    cur_thread = this_thr;						\
    if (old_cur_thread && old_cur_thread != cur_thread)			\
      error ("Already called by another thread %O (this is %O):\n"	\
	     "%s----------\n",						\
	     old_cur_thread, cur_thread,				\
	     describe_backtrace (old_cur_thread->backtrace()));		\
    lock = MyLock (old_cur_thread);					\
  }
#define THREAD_UNLOCK destruct (lock)

#else  // !THREAD_DEBUG

#define THREAD_CHECK
#define THREAD_UNLOCK

#endif

static string read_buffer; /* Data that is received before there is any
		     * read_callback 
                     * Data is also buffered here if a blocking read from HLP
		     * doesnt want to read a full packet of data.*/
string write_buffer; /* Data to be written */
function(mixed,string:void) read_callback;
function(mixed:void) write_callback;
function(mixed:void) close_callback;
function(mixed:void) accept_callback;

int connected; /* 1 if the connect callback has been called */
int blocking;  /* 1 if in blocking mode.
		* So far, there's no true blocking i/o, read and write
		* requests are just queued up. */
int is_closed;

int query_fd()
{
  return -1;
}

private void ssl_write_callback(mixed id);

#define CALLBACK_MODE							\
  (read_callback || write_callback || close_callback || accept_callback)

void die(int status)
{
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.sslfile->die: is_closed = %d\n", is_closed));
#endif
  THREAD_CHECK;
  if (status < 0)
  {
    /* Other end closed without sending a close_notify alert */
    context->purge_session(this_object());
#ifdef SSL3_DEBUG
    werror("SSL.sslfile: Killed\n");
#endif
  }
  is_closed = 1;
  if (socket)
  {
    mixed id;
    mixed err = catch(id = socket->query_id());
#ifdef DEBUG
    if (err) master()->handle_error (err);
#endif
    //werror ("%d %O %d closing socket %O\n", id, this_thread(), __LINE__, socket);
    err = catch(socket->close());
#ifdef DEBUG
    if (err) master()->handle_error (err);
#endif
    socket = 0;
    // Avoid recursive calls if the close callback closes the socket.
    if (close_callback) {
      function(mixed:void) f = close_callback;
      close_callback = 0;
      THREAD_UNLOCK;
      f (id);
    }
  }
}

/* Return 0 if the connection is still alive,
   * 1 if it was closed politely, and -1 if it died unexpectedly
   */
private int queue_write()
{
  int|string data = to_write();
#ifdef SSL3_DEBUG_TRANSPORT
  werror(sprintf("SSL.sslfile->queue_write: '%O'\n", data));
#else
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.sslfile->queue_write: '%O'\n", stringp(data)?(string)sizeof(data):data));
#endif
#endif
  if (stringp(data))
    write_buffer += data;
#ifdef SSL3_DEBUG_TRANSPORT
  werror(sprintf("SSL.sslfile->queue_write: buffer = '%O'\n", write_buffer));
#else
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.sslfile->queue_write: buffer = %O\n", sizeof(write_buffer)));
#endif
#endif

  if(sizeof(write_buffer) && CALLBACK_MODE) {
    if (mixed err = catch {
      socket->set_write_callback(ssl_write_callback);
    }) {
#ifdef DEBUG
      master()->handle_error (err);
#endif
      return(0);
    }
  }
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->queue_write: end\n");
#endif
  return stringp(data) ? 0 : data;
}

void close()
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->close\n");
#endif
  THREAD_CHECK;

  if (is_closed || !socket) return;
  is_closed = 1;

#if 0
  if (sizeof (write_buffer) && !blocking)
    ssl_write_callback(socket->query_id());

  if(sizeof(write_buffer) && blocking) {
    write_blocking();
  }
#endif

  send_close();
  queue_write();
  read_callback = 0;
  write_callback = 0;
  close_callback = 0;

  if (socket) {
    if (sizeof (write_buffer) && !blocking) {
      ssl_write_callback(socket->query_id());
    }
    if(sizeof(write_buffer) && blocking) {
      write_blocking();
    }

    if (!blocking)
    {
      // FIXME: Timeout?
      return;
    }

    if (socket) {
      //werror ("%d %O %d closing socket %O\n", id, this_thread(), __LINE__, socket);
      socket->close();
    }
  }
  socket = 0;
}


string|int read(string|int ...args) {

#ifdef SSL3_DEBUG
  werror(sprintf("sslfile.read called!, with args: %O \n",args));
#endif
  THREAD_CHECK;
  
  int nbytes;
  int notall;
  switch(sizeof(args)) {
  case 0:
    {
      string res="";
      string data=got_data(read_blocking_packet());
      
      while(stringp(data)) {
	res+=data;
	data=got_data(read_blocking_packet());
      } 
      die(1);
      return read_buffer+res;
    }
  case 1:
    {
      nbytes=args[0];
      notall=0;
      break;
    }
  case 2:
    {
      nbytes=args[0];
      notall=args[1];
      break;
    }

  default:

    error( "SSL.sslfile->read: Wrong number of arguments\n" );
  }
  
  string res="";
  int leftToRead=nbytes;

  if(strlen(read_buffer)) {
    if(leftToRead<=strlen(read_buffer)) {
      res=read_buffer[..leftToRead-1];
      read_buffer=read_buffer[leftToRead..];
      return res;
    } else {
      res=read_buffer;
      leftToRead-=strlen(read_buffer);
      if(notall) {
	return res;
      }
    }
  }

  string|int data=got_data(read_blocking_packet());
  if(!stringp(data)) {
    return ""; //EOF or ssl-fatal error occured.
  }

  while(stringp(data)) {
    res+=data;
    leftToRead-=strlen(data);
    if(leftToRead<=0) break;
    if(notall) return res;
    data=got_data(read_blocking_packet());
  }

  if(leftToRead<0) {
    read_buffer=data[strlen(data)+leftToRead..];
    return res[0..args[0]-1];
  } else {
    read_buffer="";
  }

  if(!stringp(data)) {
    die(1);
  }

  return res;
}


int write(string|array(string) s)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->write\n");
#endif
  THREAD_CHECK;

  if (is_closed || !socket) return -1;

  if (arrayp(s)) {
    s = s*"";
  }

  int call_write = !sizeof (write_buffer);
  int len = strlen(s);
  object packet;
  int res;
  while(strlen(s))
  {
    packet = Packet();
    packet->content_type = PACKET_application_data;
    packet->fragment = s[..PACKET_MAX_SIZE-1];
    send_packet(packet);
    s = s[PACKET_MAX_SIZE..];
  }

  if (blocking)
    write_blocking();
  else {
    if (call_write && socket)
      ssl_write_callback(socket->query_id());
  }
  return len;
}

 
void get_blocking_to_handshake_finished_state() {
  
  while(socket && !handshake_finished) {
    write_blocking();
    string|int s=got_data(read_blocking_packet());
    if(s==1||s==-1) break;
  }
}

// Reads a single record layer packet.
private int|string read_blocking_packet() {

  if (!socket) return -1;  

  string header=socket->read(5);
  if(!stringp(header)) {
    return -1;    
  }
  
  if(strlen(header)!=5) {
    return 1;
  }
  
  int compressedLen=header[3]*256+header[4];
  return header+socket->read(compressedLen);
}


// Writes out everything that is enqued for writing.
private void write_blocking() {

  int res = queue_write();

  while(strlen(write_buffer) && socket) {
    
    //werror ("%d %O write_blocking write beg %O\n", id, this_thread(), socket);
    int written = socket->write(write_buffer);
    //werror ("%d %O write_blocking write end %O\n", id, this_thread(), socket);
    if (written > 0) {
      write_buffer = write_buffer[written ..];
    } else {
      if (written < 0) {
	die(-1);
	return;
      }
    }
    res=queue_write();
  }
}



private void ssl_read_callback(mixed id, string s)
{
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.sslfile->ssl_read_callback, connected=%d, handshake_finished=%d\n", connected, handshake_finished));
#endif
  THREAD_CHECK;
  string|int data = got_data(s);
  if (socket) {
    int res = queue_write();
    if (res) {
      die(res);
      return;
    }
  }
  if (stringp(data))
  {
#ifdef SSL3_DEBUG
    werror(sprintf("SSL.sslfile->ssl_read_callback: application_data: '%O'\n", data));
#endif
    read_buffer += data;
    if (!connected && handshake_finished)
      {
      connected = 1;
      if (accept_callback) {
	THREAD_UNLOCK;
	accept_callback(this_object());
      }
    }
    
    if (!blocking && read_callback && strlen(read_buffer))
      {
	string received = read_buffer;
	read_buffer = "";
	THREAD_UNLOCK;
	read_callback(id, received);
      }
  } else {
    if (data > 0)
      {
	if (close_callback) {
	  function f = close_callback;
	  close_callback = 0;
	  THREAD_UNLOCK;
	  f(socket->query_id());
	}
      }
    else
      if (data < 0)
	{
	  /* Fatal error, remove from session cache */
	  if (this_object()) {
	    die(-1);
	  }
	  return;
	}
  }
}

private void ssl_write_callback(mixed id)
{
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.sslfile->ssl_write_callback: handshake_finished = %d\n"
		 "blocking = %d, write_callback = %O\n",
		 handshake_finished, blocking, write_callback));
#endif
  THREAD_CHECK;

  if (strlen(write_buffer))
    {
      //werror ("%d %O ssl_write_callback write beg %O\n", id, this_thread(), socket);
      int written = socket->write(write_buffer);
      //werror ("%d %O ssl_write_callback write end %O\n", id, this_thread(), socket);
    if (written > 0)
    {
      write_buffer = write_buffer[written ..];
    } else {
      if (written < 0)
#ifdef __NT__
	// You don't want to know.. (Bug observed in Pike 0.6.132.)
	if (socket->errno() != 1)
#endif
	{
	  die(-1);
	  return;
	}
    }
    if (strlen(write_buffer))
      return;
  }

  if (!this_object()) {
    // We've been destructed.
    return;
  }
  int res = queue_write();
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.sslport->ssl_write_callback: res = '%O'\n", res));
#endif
  
  if (strlen(write_buffer))
    return;

  if (socket)
  {
    socket->set_write_callback(0);
    if (is_closed)
    {
      //werror ("%d %O %d closing socket %O\n", id, this_thread(), __LINE__, socket);
      socket->close();
      socket = 0;
      return;
    }
  }
  if (res) {
    die(res);
    return;
  }

  if (connected && !blocking && write_callback && handshake_finished)
  {
#ifdef SSL3_DEBUG
    werror("SSL.sslport->ssl_write_callback: Calling write_callback\n");
#endif
    THREAD_UNLOCK;
    write_callback(id);
#if 0
    if (!socket || !this_object()) {
      // We've been closed or destructed.
      return;
    }
    res = queue_write();

    if (strlen(write_buffer))
      return;
#endif
  }
}

private void ssl_close_callback(mixed id)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslport: ssl_close_callback\n");
#endif
  THREAD_CHECK;
  if (close_callback && socket) {
    function f = close_callback;
    close_callback = 0;
    THREAD_UNLOCK;
    f(socket->query_id());
  }
  if (this_object()) {
    die(closing ? 1 : -1);
  }
}

static void update_callbacks()
{
  if (CALLBACK_MODE) {
    socket->set_read_callback (ssl_read_callback);
    socket->set_write_callback ((write_callback || sizeof (write_buffer)) &&
				ssl_write_callback);
    socket->set_close_callback (ssl_close_callback);
  }
  else {
    socket->set_read_callback (0);
    socket->set_write_callback (0);
    socket->set_close_callback (0);
  }
}

void set_accept_callback(function(mixed:void) a)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslport: set_accept_callback\n");
#endif
  THREAD_CHECK;
  accept_callback = a;
  if (socket) update_callbacks();
}

function query_accept_callback() { return accept_callback; }

void set_read_callback(function(mixed,string:void) r)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslport: set_read_callback\n");
#endif
  THREAD_CHECK;
  read_callback = r;
#if 0
  if (strlen(read_buffer)&& socket)
    ssl_read_callback(socket->query_id(), "");
#endif
  if (socket) update_callbacks();
}

function query_read_callback() { return read_callback; }

void set_write_callback(function(mixed:void) w)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->set_write_callback\n");
#endif
  THREAD_CHECK;
  write_callback = w;
  if (socket) update_callbacks();
}

function query_write_callback() { return write_callback; }

void set_close_callback(function(mixed:void) c)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslport: set_close_callback\n");
#endif
  THREAD_CHECK;
  close_callback = c;
  if (socket) update_callbacks();
}

function query_close_callback() { return close_callback; }

void set_nonblocking(function ...args)
{
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.sslfile->set_nonblocking(%O)\n", args));
#endif
  THREAD_CHECK;

  if (is_closed || !socket) return;

  switch (sizeof(args))
  {
  case 0:
    break;
  case 3:
    read_callback = args[0];
    write_callback = args[1];
    close_callback = args[2];
    if (!this_object())
      return;
    break;
  default:
    error( "SSL.sslfile->set_nonblocking: Wrong number of arguments\n" );
  }
  blocking = 0;
  if (is_closed || !socket) return;
  if (CALLBACK_MODE) {
    THREAD_UNLOCK;
    socket->set_nonblocking(ssl_read_callback,ssl_write_callback,
			    ssl_close_callback);
  }
  else {
    THREAD_UNLOCK;
    socket->set_nonblocking();
  }
#if 0
  if (strlen(read_buffer))
    ssl_read_callback(socket->query_id(), "");
#endif
}

void set_blocking()
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->set_blocking\n");
#endif
  THREAD_CHECK;
  blocking = 1;

  if( !socket )
    return;
  socket->set_blocking();
  if ( sizeof (write_buffer) )
    ssl_write_callback(socket->query_id());
  get_blocking_to_handshake_finished_state();
}

string query_address(int|void arg)
{
  return socket->query_address(arg);
}

int errno()
{
  // FIXME: The errno returned here might not be among the expected
  // types if we emulate blocking.
  return socket ? socket->errno() : System.EBADF;
}

void create(object f, object c, int|void is_client, int|void is_blocking)
{
#ifdef SSL3_DEBUG
  werror("SSL.sslfile->create\n");
#endif
  THREAD_CHECK;
  _fd=f->_fd;
  read_buffer = write_buffer = "";
  socket = f;
  connection::create(!is_client, c);
  blocking=is_blocking;
  if(blocking) {
    socket->set_blocking();
    get_blocking_to_handshake_finished_state();
  } else {
    THREAD_UNLOCK;
    socket->set_nonblocking(ssl_read_callback,
			  ssl_write_callback,
			  ssl_close_callback);
  }
}

string _sprintf(int t) {
  return t=='O' && sprintf("%O(%O,%O)", this_program, _fd, context);
}

void renegotiate()
{
  THREAD_CHECK;
  expect_change_cipher = certificate_state = 0;
  if (!socket) return;
  send_packet(hello_request());
  if (CALLBACK_MODE) socket->set_write_callback(ssl_write_callback);
  handshake_finished = 0;
  connected = 0;
}
