#pike __REAL_VERSION__
#require constant(SSL.Cipher)

//! Interface similar to @[Stdio.File].
//!
//! @ul
//! @item
//!   Handles blocking and nonblocking mode.
//! @item
//!   Handles callback mode in an arbitrary backend (also in blocking mode).
//! @item
//!   Read and write operations may each do both reading and
//!   writing. In callback mode that means that installing either a
//!   read or a write callback may install both internally.
//! @item
//!   In Pike 8.0 and later, blocking read and write in concurrent threads
//!   is supported.
//! @item
//!   Callback changing operations like @[set_blocking] and
//!   @[set_nonblocking] aren't atomic.
//! @item
//!   Apart from the above, thread safety/atomicity characteristics
//!   are retained.
//! @item
//!   Blocking characterstics are retained for all functions.
//! @item
//!   @[is_open], connection init (@[create]) and close (@[close]) can
//!   do both reading and writing.
//! @item
//!   @[destroy] attempts to close the stream properly by sending the
//!   close packet, but since it can't do blocking I/O it's not
//!   certain that it will succeed. The stream should therefore always
//!   be closed with an explicit @[close] call.
//! @item
//!   Abrupt remote close without the proper handshake gets the errno
//!   @[System.EPIPE].
//! @item
//!   Objects do not contain cyclic references, so they are closed and
//!   destructed timely when dropped.
//! @endul

// #define SSLFILE_DEBUG
// #define SSL3_DEBUG
// #define SSL3_DEBUG_MORE
// #define SSL3_DEBUG_TRANSPORT

#ifdef SSL3_DEBUG
protected string stream_descr;
#define SSL3_DEBUG_MSG(X...)						\
  werror ("[thr:" + this_thread()->id_number() +			\
	  "," + (stream ? "" : "ex ") + stream_descr + "] " + X)
#ifdef SSL3_DEBUG_MORE
#define SSL3_DEBUG_MORE_MSG(X...) SSL3_DEBUG_MSG (X)
#endif
#else
#define SSL3_DEBUG_MSG(X...) 0
#endif

#ifndef SSL3_DEBUG_MORE_MSG
#define SSL3_DEBUG_MORE_MSG(X...) 0
#endif

protected Stdio.File stream;
// The stream is closed by shutdown(), which is called directly or
// indirectly from destroy() or close() but not from anywhere else.
//
// Note that a close in nonblocking callback mode might not happen
// right away. In that case stream remains set after close() returns,
// suitable callbacks are installed for the close packet exchange, and
// close_state >= NORMAL_CLOSE. The stream is closed by the callbacks
// as soon the close packets are done, or if an error occurs.

protected int(-1..65535) linger_time = -1;
// The linger behaviour set by linger().

protected .Context context;
// The context to use.

protected .Connection conn;
// Always set when stream is. Destructed with destroy() at shutdown
// since it contains cyclic references. Noone else gets to it, though.

protected array(string) write_buffer; // Encrypted data to be written.
protected String.Buffer read_buffer; // Decrypted data that has been read.

protected int read_buffer_threshold;	// Max number of bytes to read.

protected mixed callback_id;
protected function(void|object,void|mixed:int) accept_callback;
protected function(void|mixed,void|string:int) read_callback;
protected function(void|mixed:int) write_callback;
protected function(void|mixed:int) close_callback;

protected Pike.Backend real_backend;
// The real backend for the stream.

protected Pike.Backend local_backend;
// Internally all I/O is done using callbacks. This backend is used
// in blocking mode.

protected int nonblocking_mode;

protected int fragment_max_size;
//! The max amount of data to send in each packet.
//! Initialized from the context when the object is created.

import .Constants;

protected int previous_connection_state = CONNECTION_handshaking;

protected enum CloseState {
  ABRUPT_CLOSE = -1,
  STREAM_OPEN = 0,
  STREAM_UNINITIALIZED = 1,
  NORMAL_CLOSE = 2,		// The caller has requested a normal close.
  CLEAN_CLOSE = 3,		// The caller has requested a clean close.
}
protected CloseState close_state = STREAM_UNINITIALIZED;
// ABRUPT_CLOSE is set if there's a remote close without close packet.
// The stream is still considered open locally, but reading or writing
// to it trigs System.EPIPE.

protected int local_errno;
// If nonzero, override the errno on the stream with this.

protected int read_errno;
protected int write_errno;
protected int close_errno;
// Stores the errno from failed I/O in a callback so that the next
// visible I/O operation can report it properly.

protected int alert_cb_called;
// Need to know if the alert callback has been called in
// ssl_read_callback since it can't continue in that case. This is
// only set temporarily while ssl_read_callback runs.

protected constant epipe_errnos = (<
  System.EPIPE,
  System.ECONNRESET,
#if constant(System.WSAECONNRESET)
  // The following is returned by winsock on windows.
  // Pike ought to map it to System.ECONNRESET.
  System.WSAECONNRESET,
#endif
>);
// Multiset containing the errno codes that can occur if the remote
// end has closed the connection.

// Ignore user installed callbacks if a close has been requested locally.
#define CALLBACK_MODE							\
  ((read_callback || write_callback || close_callback || accept_callback) && \
   close_state < NORMAL_CLOSE)

#define SSL_HANDSHAKING (!conn || ((conn->state & CONNECTION_handshaking) && \
				   (close_state != ABRUPT_CLOSE)))
#define SSL_CLOSING_OR_CLOSED						\
  (conn->state & CONNECTION_local_closing)

// Always wait for input during handshaking and when we expect the
// remote end to respond to our close packet. We should also check the
// input buffer for a close packet if there was a failure to write our
// close packet.
#define SSL_INTERNAL_READING						\
  (conn && (SSL_HANDSHAKING ||						\
	    ((conn->state & CONNECTION_closed) == CONNECTION_local_closed)))

// Try to write when there's data in the write buffer or when we have
// a close packet to send. The packet is queued separately by
// ssl_write_callback in the latter case.
#define SSL_INTERNAL_WRITING (conn &&					\
			      (sizeof (write_buffer) ||			\
			       ((conn->state & CONNECTION_local_down) == \
				CONNECTION_local_closing)))

#ifdef SSLFILE_DEBUG

#if constant (Thread.thread_create)

#define THREAD_T Thread.Thread
#define THIS_THREAD() this_thread()


protected void thread_error (string msg, THREAD_T other_thread)
{
#if 0 && constant (_locate_references)
  werror ("%s\n%O got %d refs", msg, this, _refs (this));
  _locate_references (this);
#endif
  error ("%s"
	 "%s\n"
	 "User callbacks: a=%O r=%O w=%O c=%O\n"
	 "Internal callbacks: r=%O w=%O c=%O\n"
	 "Backend: %O  This thread: %O  Other thread: %O\n"
	 "%s",
	 msg,
	 !stream ? "Got no stream" :
	 stream->is_open() ? "Stream is open" :
	 "Stream is closed",
	 accept_callback, read_callback, write_callback, close_callback,
	 stream && stream->query_read_callback(),
	 stream && stream->query_write_callback(),
	 stream && stream->query_close_callback(),
	 stream && stream->query_backend(),
	 this_thread(), other_thread,
	 other_thread ? ("Other thread backtrace:\n" +
			 describe_backtrace (other_thread->backtrace()) +
			 "----------\n") : "");
}

#else  // !constant (Thread.thread_create)

#define THREAD_T int
#define THIS_THREAD() 1

protected void thread_error (string msg, THREAD_T other_thread)
{
  error ("%s"
	 "%s\n"
	 "User callbacks: a=%O r=%O w=%O c=%O\n"
	 "Internal callbacks: r=%O w=%O c=%O\n"
	 "Backend: %O\n",
	 msg,
	 !stream ? "Got no stream" :
	 stream->is_open() ? "Stream is open" :
	 "Stream is closed",
	 accept_callback, read_callback, write_callback, close_callback,
	 stream && stream->query_read_callback(),
	 stream && stream->query_write_callback(),
	 stream && stream->query_close_callback(),
	 stream && stream->query_backend());
}

#endif	// !constant (Thread.thread_create)

#endif	// !SSLFILE_DEBUG

#define CHECK_CB_MODE(CUR_THREAD) do {} while (0)
#define CHECK(IN_CALLBACK) do {} while (0)
#define ENTER(IN_CALLBACK) do
#define RESTORE do {} while (0)
#define RETURN(RET_VAL) return (RET_VAL)
#define LEAVE while (0)


//! Run one pass of the backend.
protected int(0..0)|float backend_once()
{
  if (nonblocking_mode) {
    // Assume that the backend is active when is has been started.
    if (master()->asyncp()) return 0;
#if constant(thread_create)
    // Only run the main backend from the backend thread.
    if (master()->backend_thread() != this_thread()) return 0;
#endif
    // NB: The following happens when the real backend is run
    //     by the user by hand (like eg the testsuite, or
    //     when nesting several levels of SSL.File objects).
    if (real_backend->executing_thread()) return 0;

    SSL3_DEBUG_MSG("Running real backend [r:%O w:%O], zero timeout\n",
		   !!stream->query_read_callback(),
		   !!stream->query_write_callback());
    return real_backend(0.0);
  } else {
#if constant(thread_create)
    if (local_backend->executing_thread() == this_thread()) return 0;
#else
    if (local_backend->executing_thread()) return 0;
#endif
    SSL3_DEBUG_MSG("Running local backend [r:%O w:%O], infinite timeout\n",
		   !!stream->query_read_callback(),
		   !!stream->query_write_callback());
    return local_backend(0);
  }
}

// stream is assumed to be operational on entry but might be zero
// afterwards.
#define RUN_MAYBE_BLOCKING(REPEAT_COND, NONWAITING_MODE) do {		\
  run_local_backend: {							\
      CHECK_CB_MODE (THIS_THREAD());					\
									\
      while (1) {							\
	float|int(0..0) action;						\
									\
	if (conn->state & CONNECTION_peer_fatal) {			\
	  SSL3_DEBUG_MSG ("Backend ended efter peer fatal.\n");		\
	  break;							\
	}								\
									\
	action = backend_once();					\
									\
	if (NONWAITING_MODE && !action) {				\
	  SSL3_DEBUG_MSG ("Nonwaiting local backend ended - nothing to do\n"); \
	  break;							\
	}								\
									\
	if (!action && (conn->state & CONNECTION_local_closing)) {	\
	  SSL3_DEBUG_MSG ("Did not get a remote close - "		\
			  "signalling delayed error from writing close message\n"); \
	  cleanup_on_error();						\
	  close_errno = write_errno = System.EPIPE;			\
	  if (close_state != CLEAN_CLOSE)				\
	    close_state = ABRUPT_CLOSE;					\
	}								\
									\
	if (!stream) {							\
	  SSL3_DEBUG_MSG ("Backend ended after close.\n");		\
	  break run_local_backend;					\
	}								\
									\
	if (!(REPEAT_COND)) {						\
	  SSL3_DEBUG_MSG ("Local backend ended - repeat condition false\n"); \
	  break;							\
	}								\
      }									\
									\
      CHECK_CB_MODE (THIS_THREAD());					\
    }									\
  } while (0)

protected void create (Stdio.File stream, SSL.Context ctx)
//! Create an SSL connection over an open @[stream].
//!
//! @param stream
//!   Open socket or pipe to create the connection over.
//!
//! @param ctx
//!   The SSL context.
//!
//! The backend used by @[stream] is taken over and restored after the
//! connection is closed (see @[close] and @[shutdown]). The callbacks
//! and id in @[stream] are overwritten.
//!
//! @note
//!   The operation mode defaults to nonblocking mode.
//!
//! @seealso
//!   @[accept()], @[connect()]
{
  SSL3_DEBUG_MSG ("SSL.File->create (%O, %O)\n", stream, ctx);

  ENTER (0) {
    global::stream = stream;
    global::context = ctx;

#ifdef SSL3_DEBUG
    if (stream->query_fd)
      stream_descr = "fd:" + stream->query_fd();
    else
      stream_descr = replace (sprintf ("%O", stream), "%", "%%");
#endif
    write_buffer = ({});
    read_buffer = String.Buffer();
    read_buffer_threshold = Stdio.DATA_CHUNK_SIZE;
    real_backend = stream->query_backend();
    close_state = STREAM_OPEN;

    stream->set_read_callback (0);
    stream->set_write_callback (0);
    stream->set_close_callback (0);
    stream->set_id (0);

    fragment_max_size =
      limit(1, ctx->packet_max_size, PACKET_MAX_SIZE);

    set_nonblocking();
  } LEAVE;
}

//! Configure as client and set up the connection.
//!
//! @param dest_addr
//!   Optional name of the server that we are connected to.
//!
//! @returns
//!   Returns @expr{0@} on handshaking failure in blocking mode,
//!   and otherwise @expr{1@}.
//!
//! @seealso
//!   @[accept()]
int(1bit) connect(string|void dest_addr)
{
  if (conn) error("A connection is already configured!\n");

  ENTER (0) {

    conn = .ClientConnection(context, dest_addr);

    SSL3_DEBUG_MSG("connect: Installing read/close callbacks.\n");

    stream->set_read_callback(ssl_read_callback);
    stream->set_close_callback(ssl_close_callback);

    // Wait for the handshake to finish in blocking mode.
    if (!nonblocking_mode) {
      if (!direct_write()) {
	local_errno = errno();
	if (stream) {
	  stream->set_callbacks(0, 0, 0, 0, 0);
	}
	conn = UNDEFINED;
	return 0;
      }
    } else {
      queue_write();
    }
  } LEAVE;

  return 1;
}

//! Configure as server and set up the connection.
//!
//! @param pending_data
//!   Any data that has already been read from the stream.
//!   This is typically used with protocols that use
//!   START TLS or similar, where there's a risk that
//!   "too much" data (ie part of the TLS ClientHello) has
//!   been read from the stream before deciding that the
//!   connection is to enter TLS-mode.
//!
//! @returns
//!   Returns @expr{0@} on handshaking failure in blocking mode,
//!   and otherwise @expr{1@}.
//!
//! @seealso
//!   @[connect()]
int(1bit) accept(string|void pending_data)
{
  if (conn) error("A connection is already configured!\n");

  ENTER (0) {
    conn = .ServerConnection(context);

    if (sizeof(pending_data || "")) {
      if (intp(conn->got_data(pending_data))) {
	local_errno = errno();
	if (stream) {
	  stream->set_callbacks(0, 0, 0, 0, 0);
	}
	conn = UNDEFINED;
	return 0;
      }
    }

    SSL3_DEBUG_MSG("accept: Installing read/close callbacks.\n");

    stream->set_read_callback(ssl_read_callback);
    stream->set_close_callback(ssl_close_callback);

    // Wait for the handshake to finish in blocking mode.
    if (!nonblocking_mode) {
      if (!direct_write()) {
	local_errno = errno();
	if (stream) {
	  stream->set_callbacks(0, 0, 0, 0, 0);
	}
	conn = UNDEFINED;
	return 0;
      }
    }
  } LEAVE;

  return 1;
}

mixed get_server_name()
{
  if (!conn) error("No active conection.\n");
  return conn->session->server_name;
}

//! @returns
//!   Returns peer certificate information, if any.
mapping get_peer_certificate_info()
{
  if (!conn) error("No active conection.\n");
  return conn->session->cert_data;
}

//! @returns
//!   Returns the peer certificate chain, if any.
array get_peer_certificates()
{
  if (!conn) error("No active conection.\n");
  return conn->session->peer_certificate_chain;
}

//! Set the linger time on @[close()].
int(0..1) linger(int(-1..65535)|void seconds)
{
  if (!stream) return 0;
  if (zero_type(seconds)) seconds = -1;
  if (seconds == linger_time) {
    // Noop.
    return 1;
  }
  if (stream->linger && !stream->linger(seconds)) return 0;
  linger_time = seconds;
  return 1;
}

int close (void|string how, void|int clean_close, void|int dont_throw)
//! Close the connection. Both the read and write ends are always
//! closed
//!
//! @param how
//!   This argument is only for @[Stdio.File] compatibility
//!   and must be either @expr{"rw"@} or @expr{0@}.
//!
//! @param clean_close
//!   If set then close messages are exchanged to shut down
//!   the SSL connection but not the underlying stream. It may then
//!   continue to be used for other communication afterwards. The
//!   default is to send a close message and then close the stream
//!   without waiting for a response.
//!
//! @param dont_throw
//!   I/O errors are normally thrown, but that can be turned off with
//!   @[dont_throw]. In that case @[errno] is set instead and @expr{0@} is
//!   returned. @expr{1@} is always returned otherwise. It's not an error to
//!   close an already closed connection.
//!
//! @note
//! If a clean close is requested in nonblocking mode then the stream
//! is most likely not closed right away, and the backend is then
//! still needed for a while afterwards to exchange the close packets.
//! @[is_open] returns 2 in that time window.
//!
//! @note
//! I/O errors from both reading and writing might occur in blocking
//! mode.
//!
//! @note
//! If a clean close is requested and data following the close message
//! is received at the same time, then this object will read it and
//! has no way to undo that. That data can be retrieved with @[read]
//! afterwards.
//!
//! @seealso
//!   @[shutdown]
{
  SSL3_DEBUG_MSG ("SSL.File->close (%O, %O, %O)\n",
		  how, clean_close, dont_throw);

  if (how && how != "rw")
    error ("Can only close the connection in both directions simultaneously.\n");

  local_errno = 0;

  ENTER (0) {
    if (!conn || (conn->state & CONNECTION_local_down)) {
      SSL3_DEBUG_MSG ("SSL.File->close: Already closed (%d)\n", close_state);
      RETURN (1);
    }
    close_state = clean_close ? CLEAN_CLOSE : NORMAL_CLOSE;

    if (close_errno) {
      local_errno = close_errno;

      SSL3_DEBUG_MSG ("SSL.File->close: Shutdown after error\n");
      int err = errno();
      shutdown();
      // Get here e.g. if a close callback calls close after an
      // error, so never throw. (I'm somewhat suspicious to this,
      // but I guess I did it with good reason.. :P /mast)
      local_errno = err;
      RETURN (0);
    }

    SSL3_DEBUG_MSG ("ssl_write_callback: Queuing close packet\n");
    conn->send_close();
    if (!linger_time) {
      SSL3_DEBUG_MSG ("ssl_write_callback: Don't care about it being sent.\n");
      conn->state = [int(0..0)|ConnectionState]
	(conn->state | CONNECTION_local_closed);
    }

    // Even in nonblocking mode we call direct_write here to try to
    // put the close packet in the send buffer before we return. That
    // way it has a fair chance to get sent even when we're called
    // from destroy() (in which case it won't work to just install the
    // write callback as usual and wait for the backend to call it).

    if (!direct_write() || close_errno) {
      SSL3_DEBUG_MSG("SSL.File->close(): direct_write failed.\n"
		     "local_errno: %d, write_errno: %d, close_errno: %d\n",
		     local_errno, write_errno, close_errno);
      // Should be shut down after close(), even if an error occurred.
      int err = close_errno;
      shutdown();
      local_errno = err;
      if (dont_throw) {
	local_errno = err;
	RETURN (0);
      }
      else if( err != System.EPIPE )
	// Errors are normally thrown from close().
        error ("Failed to close SSL connection: %s\n", strerror (err));
    }

    if (stream && (stream->query_read_callback() || stream->query_write_callback())) {
      SSL3_DEBUG_MSG ("SSL.File->close: Close underway\n");
      RESTORE;
      return 1;
    }
    else {
      // The local backend run by direct_write has typically already
      // done this, but it might happen that it doesn't do anything in
      // case close packets already have been exchanged.
      shutdown();
      SSL3_DEBUG_MSG ("SSL.File->close: Close done\n");
    }
  } LEAVE;
  return 1;
}

protected void cleanup_on_error()
// Called when any error occurs on the stream.
{
  // The session should be purged when an error has occurred.
  if (conn && conn->session)
    // Check conn->session since it doesn't exist before the
    // handshake.
    conn->context->purge_session (conn->session);
}

Stdio.File shutdown()
//! Shut down the SSL connection without sending any more packets.
//!
//! If the connection is open then the underlying (still open) stream
//! is returned.
//!
//! If a nonclean (i.e. normal) close has been requested then the
//! underlying stream is closed now if it wasn't closed already, and
//! zero is returned.
//!
//! If a clean close has been requested (see the second argument to
//! @[close]) then the behavior depends on the state of the close
//! packet exchange: The first @[shutdown] call after a successful
//! exchange returns the (still open) underlying stream, and later
//! calls return zero and clears @[errno]. If the exchange hasn't
//! finished then the stream is closed, zero is returned, and @[errno]
//! will return @[System.EPIPE].
//!
//! @seealso
//!   @[close], @[set_alert_callback]
{
  ENTER (0) {
    if (!stream || !conn) {
      SSL3_DEBUG_MSG ("SSL.File->shutdown(): Already shut down\n");
      RETURN (0);
    }

    if (close_state == STREAM_OPEN || close_state == NORMAL_CLOSE)
      // If we didn't request a clean close then we pretend to have
      // received a close message. According to the standard it's ok
      // anyway as long as the transport isn't used for anything else.
      conn->state |= CONNECTION_peer_closed;

    SSL3_DEBUG_MSG ("SSL.File->shutdown(): %s %s\n",
		    close_state != ABRUPT_CLOSE &&
		    (conn->state &
		     (CONNECTION_peer_closed | CONNECTION_local_closing)) ==
		    (CONNECTION_peer_closed | CONNECTION_local_closing) &&
		    !sizeof (write_buffer) ?
		    "Proper close" :
		    close_state == STREAM_OPEN ?
		    "Not closed" :
		    "Abrupt close",
		    conn->describe_state());

    if ((conn->state & CONNECTION_peer_closed) &&
	sizeof (conn->left_over || "")) {
#ifdef SSLFILE_DEBUG
      werror ("Warning: Got buffered data after close in %O: %O%s\n", this,
	      conn->left_over[..99], sizeof (conn->left_over) > 100 ? "..." : "");
#endif
      read_buffer = String.Buffer (sizeof (conn->left_over));
      read_buffer->add (conn->left_over);
      close_state = STREAM_OPEN;
    }

    if (conn->state & CONNECTION_local_failing) {
      if (close_state != ABRUPT_CLOSE) {
	SSL3_DEBUG_MSG("Local failure -- force ABRUPT CLOSE.\n");
      }
      close_state = ABRUPT_CLOSE;
    }

    .Constants.ConnectionState conn_state = conn->state;
    destruct (conn);		// Necessary to avoid garbage.

    write_buffer = ({});

    if (user_cb_co) {
      real_backend->remove_call_out(user_cb_co);
      user_cb_co = UNDEFINED;
    }

    Stdio.File stream = global::stream;
    global::stream = 0;

    // Zapp all the callbacks.
    stream->set_nonblocking();
    // Restore the configured backend.
    stream->set_backend(real_backend);

    switch (close_state) {
      case CLEAN_CLOSE:
	if ((conn_state & CONNECTION_closed) == CONNECTION_closed) {
	  SSL3_DEBUG_MSG ("SSL.File->shutdown(): Clean close - "
			  "leaving stream\n");
	  local_errno = 0;
	  RETURN (stream);
	}
	else {
	  SSL3_DEBUG_MSG ("SSL.File->shutdown(): Close packets not fully "
			  "exchanged after clean close (%d) - closing stream\n",
			  conn_state);
	  stream->close();
	  local_errno = System.EPIPE;
	  RETURN (0);
	}
      case STREAM_OPEN:
	close_state = STREAM_UNINITIALIZED;
	SSL3_DEBUG_MSG ("SSL.File->shutdown(): Not closed - leaving stream\n");
	local_errno = 0;
	if (!nonblocking_mode) stream->set_blocking();
	RETURN (stream);
      default:
	SSL3_DEBUG_MSG ("SSL.File->shutdown(): Nonclean close - closing stream\n");
	// if (stream->linger) stream->linger(0);
	stream->close();
	local_errno = stream->errno() || local_errno;
	RETURN (0);
    }
  } LEAVE;
}

protected void destroy()
//! Try to close down the connection properly since it's customary to
//! close files just by dropping them. No guarantee can be made that
//! the close packet gets sent successfully though, because we can't
//! risk blocking I/O here. You should call @[close] explicitly.
//!
//! @seealso
//!   @[close]
{
  SSL3_DEBUG_MSG ("SSL.File->destroy()\n");

  // We don't know which thread this will be called in if the refcount
  // garb or the gc got here. That's not a race problem since it won't
  // be registered in a backend in that case.
  if (stream) {
    // Make sure not to fail in ENTER below due to bad backend thread.
    // [bug 6958].
    stream->set_callbacks(0, 0, 0);
  }
  ENTER (0) {
    if (stream) {
      if (close_state == STREAM_OPEN &&
	  // Don't bother with closing nicely if there's an error from
	  // an earlier operation. close() will throw an error for it.
	  !close_errno) {
	// We can't use our set_nonblocking() et al here, since we
	// might be associated with a backend in a different thread,
	// and update_internal_state() will install callbacks, which
	// in turn might trigger the tests in CHECK_CB_MODE().
	stream->set_nonblocking();	// Make sure not to to block.
	nonblocking_mode = 0;	// Make sure not to install any callbacks.
	close (0, 0, 1);
      }
      else
	shutdown();
    }
  } LEAVE;

  // We don't want the callback to be called after we are gone...
  if (user_cb_co) real_backend->remove_call_out(user_cb_co);
}

string read (void|int length, void|int(0..1) not_all)
//! Read some (decrypted) data from the connection. Works like
//! @[Stdio.File.read].
//!
//! @note
//! I/O errors from both reading and writing might occur in blocking
//! mode.
//!
//! @seealso
//!   @[write]
{
  SSL3_DEBUG_MSG ("SSL.File->read (%d, %d)\n", length, not_all);

  ENTER (0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    if (read_errno && !sizeof(read_buffer)) {
      local_errno = read_errno;
      SSL3_DEBUG_MSG ("SSL.File->read: Propagating old callback error: %s\n",
		      strerror (local_errno));
      RETURN (0);
    }

    local_errno = 0;

    if (zero_type(length)) length = 0x7fffffff;

    if (!nonblocking_mode && length > read_buffer_threshold) {
      // Make sure that we read as much data as requested if possible.
      read_buffer_threshold = length;
    }

    if (stream && !(conn->state & CONNECTION_peer_down)) {
      SSL3_DEBUG_MSG("SSL.File->read: Installing read_callback.\n");
      stream->set_read_callback(ssl_read_callback);

      if (not_all) {
	if (!sizeof (read_buffer))
	  RUN_MAYBE_BLOCKING (!sizeof (read_buffer) &&
			      !(conn->state & CONNECTION_peer_down), 0);
      }
      else {
	if (sizeof (read_buffer) < length)
	  RUN_MAYBE_BLOCKING ((sizeof (read_buffer) < length) &&
			      !(conn->state & CONNECTION_peer_down),
			      nonblocking_mode);
      }
    }

    string res = read_buffer->get();
    read_buffer->add (res[length..]);
    res = res[..length-1];

    read_buffer_threshold = Stdio.DATA_CHUNK_SIZE;

    if (SSL_INTERNAL_WRITING) {
      queue_write();
    }

    SSL3_DEBUG_MSG ("SSL.File->read: Read done, returning %d bytes "
		    "(%d/%d still in buffer)\n",
		    sizeof (res), sizeof (read_buffer), read_buffer_threshold);

    if (stream) {
      if ((sizeof(read_buffer) >= read_buffer_threshold) ||
	  (conn->state & CONNECTION_peer_down)) {
	SSL3_DEBUG_MSG("SSL.File->read: Removing read_callback.\n");
	stream->set_read_callback(0);
      }
    }

    if (res == "") {
      if (read_errno) {
	local_errno = read_errno;
	SSL3_DEBUG_MSG ("SSL.File->read: Propagating callback error: %s\n",
			strerror (local_errno));
	RETURN (0);
      }

      if (conn->state & CONNECTION_peer_fatal) {
	local_errno = read_errno = System.EIO;
	SSL3_DEBUG_MSG ("SSL.File->read: Connection aborted by peer.\n");
	RETURN (0);
      }

      if (nonblocking_mode &&
	  !(conn->state & CONNECTION_peer_closed)) {
	SSL3_DEBUG_MSG ("SSL.File->read: Would block.\n");
	local_errno = System.EAGAIN;
      }
    }

    RETURN (res);
  } LEAVE;
}

int write (string|array(string) data, mixed... args)
//! Write some (unencrypted) data to the connection. Works like
//! @[Stdio.File.write] except that this function often buffers some data
//! internally, so there's no guarantee that all the consumed data has
//! been successfully written to the stream in nonblocking mode. It
//! keeps the internal buffering to a minimum, however.
//!
//! @note
//! This function returns zero if attempts are made to write data
//! during the handshake phase and the mode is nonblocking.
//!
//! @note
//! I/O errors from both reading and writing might occur in blocking
//! mode.
//!
//! @seealso
//!   @[read]
{
  SSL3_DEBUG_MSG ("SSL.File->write (%t[%d]%{, %t%})\n",
		  data, sizeof (data), args);

  if (sizeof (args)) {
    data = ({ sprintf (arrayp (data) ? data * "" : data, @args) });
  } else if (stringp(data)) {
    data = ({ data });
  }

  ENTER (0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    if (write_errno) {
      local_errno = write_errno;

      SSL3_DEBUG_MSG ("SSL.File->write: Propagating old callback error: %s\n",
		      strerror (local_errno));
      RETURN (-1);
    }

    if (nonblocking_mode && SSL_HANDSHAKING) {
      local_errno = System.EAGAIN;
      SSL3_DEBUG_MSG ("SSL.File->write: "
		      "Still in handshake - cannot accept application data\n");
      RETURN (0);
    }

    // Take care of any old data first.
    if (!direct_write()) RETURN (-1);

    if (conn->state & CONNECTION_peer_fatal) {
      SSL3_DEBUG_MSG ("SSL.File->write: Connection aborted by peer.\n");
      local_errno = write_errno = System.EIO;
      RETURN (-1);
    }

    int write_limit = 0x7fffffff;
    if (nonblocking_mode) {
      // Always stop after writing DATA_CHUNK_SIZE in
      // nonblocking mode, so that we don't loop here
      // arbitrarily long if the write is very large and the
      // bottleneck is in the encryption.
      write_limit = Stdio.DATA_CHUNK_SIZE;
    }

    int written = 0;

    int idx = 0, pos = 0;
    while (idx < sizeof (data) && !sizeof (write_buffer) &&
	   written < write_limit) {
#ifdef SSL3_DEBUG
      int oidx = idx;
      int opos = pos;
#endif

      // send_streaming_data will pick the first fragment_max_size
      // bytes of the string, so do that right away in the same
      // range operation.
      string frag = data[idx][pos..pos + fragment_max_size -1];
      pos += fragment_max_size;

      while (sizeof(frag) < fragment_max_size) {
	// Try to fill a packet.
	if (++idx >= sizeof(data)) break;
	pos = fragment_max_size - sizeof(frag);
	frag += data[idx][..pos -1];
      }

      int n = conn->send_streaming_data (frag);
      if (n != sizeof(frag)) {
	error ("Unexpected fragment_max_size discrepancy wrt send_streaming_data.\n");
      }
#ifdef SSL3_DEBUG
      if (oidx == idx) {
	SSL3_DEBUG_MSG("SSL.File->write: Queued data[%d][%d..%d]\n",
		       idx, opos, pos - 1);
      } else {
	SSL3_DEBUG_MSG("SSL.File->write: Queued data[%d..%d][%d..%d]\n",
		       oidx, idx, opos, pos - 1);
      }
#endif
      written += n;

      if (!direct_write()) RETURN (written);
    }

    SSL3_DEBUG_MSG ("SSL.File->write: Write %t done, accepted %d bytes\n",
		    data, written);
    RETURN (written);
  } LEAVE;
}

int renegotiate()
//! Renegotiate the connection by starting a new handshake. Note that
//! the accept callback will be called again when the handshake is
//! finished.
//!
//! Returns zero if there are any I/O errors. @[errno()] will give the
//! details.
//!
//! @note
//! The read buffer is not cleared - a @expr{read()@} afterwards will
//! return data from both before and after the renegotiation.
//!
//! @bugs
//! Data in the write queue in nonblocking mode is not properly
//! written before resetting the connection. Do a blocking
//! @expr{write("")@} first to avoid problems with that.
{
  SSL3_DEBUG_MSG ("SSL.File->renegotiate()\n");

  ENTER (0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    if (read_errno) {
      local_errno = read_errno;
      SSL3_DEBUG_MSG ("SSL.File->renegotiate: "
		      "Propagating old callback error: %s\n",
		      strerror (local_errno));
      RETURN (0);
    }

    if (!stream) {
      SSL3_DEBUG_MSG ("SSL.File->renegotiate: "
		      "Connection closed - simulating System.EPIPE\n");
      cleanup_on_error();
      local_errno = System.EPIPE;
      RETURN (0);
    }

    local_errno = 0;

    // FIXME: Change this state with a packet instead so that things
    // currently in the queue aren't affect by it.
    conn->expect_change_cipher = 0;
    conn->certificate_state = 0;
    conn->state |= CONNECTION_handshaking;

    SSL3_DEBUG_MSG("renegotiate: Installing read/close callbacks.\n");

    stream->set_read_callback(ssl_read_callback);
    stream->set_close_callback(ssl_close_callback);

    conn->send_packet(conn->hello_request());

    RETURN (direct_write());
  } LEAVE;
}

//! Check whether any callbacks may need to be called.
//!
//! Always run via the @[real_backend].
//!
//! @seealso
//!   @[schedule_poll()]
protected void internal_poll()
{
  if (!this_object() || !conn) return;
  user_cb_co = UNDEFINED;
  SSL3_DEBUG_MSG("poll: %s\n", conn->describe_state());

  //
  // Close error ==> close_callback
  //
  if (conn->state & CONNECTION_local_closed) {
    if (close_callback && close_errno && close_state < NORMAL_CLOSE) {
      // Better signal errors writing the close packet to the
      // close callback.
      local_errno = close_errno;
      SSL3_DEBUG_MSG("poll: Calling close callback %O "
		     "(error %s)\n", close_callback, strerror (local_errno));
      close_callback(callback_id);
      return;
    }
  }

  //
  // Handshake completed ==> accept_callback
  //
  if (((previous_connection_state ^ conn->state) & CONNECTION_handshaking) &&
      (previous_connection_state & CONNECTION_handshaking)) {
    previous_connection_state = conn->state;
    SSL3_DEBUG_MSG ("ssl_read_callback: Handshake finished\n");
    if (accept_callback) {
      SSL3_DEBUG_MSG ("ssl_read_callback: Calling accept callback %O\n",
		      accept_callback);
      accept_callback(this, callback_id);
      return;
    }
  }
  previous_connection_state = conn->state;

  //
  // read_callback
  //
  if (sizeof(read_buffer)) {
    if (read_callback) {
      string received = read_buffer->get();
      SSL3_DEBUG_MSG ("call_read_callback: Calling read callback %O "
		      "with %d bytes\n", read_callback, sizeof (received));
      // Never called if there's an error - no need to propagate errno.
      read_callback (callback_id, received);
    }
  }

  //
  // close_callback
  //
  // NB: Don't call the close_callback before the read_buffer is empty.
  if (!sizeof(read_buffer) &&
      (conn->state &
       (CONNECTION_peer_closed | CONNECTION_local_closing)) ==
      CONNECTION_peer_closed) {
    // Remote close.

    function(void|mixed:int) close_cb;
    if (close_cb = close_callback) {
      // To correctly imitate the behavior in
      // Stdio.File (it deinstalls read side cbs whenever the close
      // cb is called, but it's possible to reinstall the close cb
      // later and get another call to it).

      SSL3_DEBUG_MSG("SSL.File->poll: Removing user close_callback.\n");
      set_close_callback(0);

      // errno() should return the error in the close callback - need to
      // propagate it here.
      local_errno = close_errno;
      SSL3_DEBUG_MSG ("ssl_read_callback: Calling close callback %O (error %s)\n",
		      close_cb, strerror(local_errno));
      close_cb(callback_id);
      return;
    }
  }

  //
  // write_callback
  //
  SSL3_DEBUG_MSG("poll: wcb: %O, cs: %d, we: %d\n",
		 write_callback, close_state, write_errno);
  if (!sizeof(write_buffer) && write_callback && !SSL_HANDSHAKING
      && (close_state < NORMAL_CLOSE || write_errno)) {
    // errno() should return the error in the write callback - need
    // to propagate it here.
    local_errno = write_errno;
    SSL3_DEBUG_MSG("poll: Calling write callback %O "
		   "(error %s)\n", write_callback, strerror (local_errno));
    write_callback(callback_id);
    return;
  }
}

protected mixed user_cb_co;

//! Schedule calling of any relevant callbacks the next
//! time the @[real_backend] is run.
//!
//! @seealso
//!   @[internal_poll()]
protected void schedule_poll()
{
  if (!user_cb_co) {
    user_cb_co = real_backend->call_out(internal_poll, 0);
  }
}

void set_callbacks (void|function(mixed, string:int) read,
		    void|function(mixed:int) write,
		    void|function(mixed:int) close,
		    void|function(mixed, string:int) read_oob,
		    void|function(mixed:int) write_oob,
		    void|function(void|mixed:int) accept)
//! Installs all the specified callbacks at once. Use @[UNDEFINED]
//! to keep the current setting for a callback.
//!
//! Like @[set_nonblocking], the callbacks are installed atomically.
//! As opposed to @[set_nonblocking], this function does not do
//! anything with the stream, and it doesn't even have to be open.
//!
//! @bugs
//! @[read_oob] and @[write_oob] are currently ignored.
//!
//! @seealso
//! @[set_read_callback], @[set_write_callback],
//! @[set_close_callback], @[set_accept_callback], @[query_callbacks]
{
  SSL3_DEBUG_MSG ("SSL.File->set_callbacks (%O, %O, %O, %O, %O, %O)\n%s",
		  read, write, close, read_oob, write_oob, accept,
		  "" || describe_backtrace (backtrace()));

  ENTER(0) {

    // Bypass the ::set_xxx_callback functions; we instead enable all
    // the event bits at once through the _enable_callbacks call at the end.

    if (!zero_type(read))
      read_callback = read;

    if (!zero_type(write))
      write_callback = write;

    if (!zero_type(close))
      close_callback = close;

    if (!zero_type(accept))
      accept_callback = accept;

    schedule_poll();
  } LEAVE;
}

//! @returns
//!   Returns the currently set callbacks in the same order
//!   as the arguments to @[set_callbacks].
//!
//! @seealso
//!   @[set_callbacks], @[set_nonblocking]
array(function(mixed,void|string:int)) query_callbacks()
{
  return ({
    read_callback,
    write_callback,
    close_callback,
    UNDEFINED,
    UNDEFINED,
    accept_callback,
  });
}

void set_nonblocking (void|function(void|mixed,void|string:int) read,
		      void|function(void|mixed:int) write,
		      void|function(void|mixed:int) close,
		      void|function(void|mixed:int) read_oob,
		      void|function(void|mixed:int) write_oob,
		      void|function(void|mixed:int) accept)
//! Set the stream in nonblocking mode, installing the specified
//! callbacks. The alert callback isn't touched.
//!
//! @note
//! Prior to version 7.5.12, this function didn't set the accept
//! callback.
//!
//! @bugs
//! @[read_oob] and @[write_oob] are currently ignored.
//!
//! @seealso
//!   @[set_callbacks], @[query_callbacks], @[set_nonblocking_keep_callbacks],
//!   @[set_blocking]
{
  SSL3_DEBUG_MSG ("SSL.File->set_nonblocking (%O, %O, %O, %O, %O, %O)\n%s",
		  read, write, close, read_oob, write_oob, accept,
		  "" || describe_backtrace (backtrace()));

  ENTER (0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    nonblocking_mode = 1;

    accept_callback = accept;
    read_callback = read;
    write_callback = write;
    close_callback = close;

    if (stream) {
      stream->set_backend(real_backend);
      stream->set_nonblocking_keep_callbacks();

      schedule_poll();

      // Has to restore here since a backend waiting in another thread
      // might be woken immediately when callbacks are registered.
      RESTORE;
      return;
    }

    schedule_poll();
  } LEAVE;
}

void set_nonblocking_keep_callbacks()
//! Set nonblocking mode like @[set_nonblocking], but don't alter any
//! callbacks.
//!
//! @seealso
//!   @[set_nonblocking], @[set_blocking], @[set_blocking_keep_callbacks]
{
  SSL3_DEBUG_MSG ("SSL.File->set_nonblocking_keep_callbacks()\n");

  ENTER (0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    nonblocking_mode = 1;

    if (stream) {
      stream->set_backend(real_backend);
      stream->set_nonblocking_keep_callbacks();
      // Has to restore here since a backend waiting in another thread
      // might be woken immediately when callbacks are registered.
      RESTORE;
      return;
    }
  } LEAVE;
}

void set_blocking()
//! Set the stream in blocking mode. All but the alert callback are
//! zapped.
//!
//! @note
//! There might be some data still waiting to be written to the
//! stream. That will be written in the next blocking call, regardless
//! what it is.
//!
//! @note
//! This function doesn't solve the case when the connection is used
//! nonblocking in some backend thread and another thread switches it
//! to blocking and starts using it. To solve that, put a call out in
//! the backend from the other thread that switches it to blocking,
//! and then wait until that call out has run.
//!
//! @note
//! Prior to version 7.5.12, this function didn't clear the accept
//! callback.
//!
//! @seealso
//!   @[set_nonblocking], @[set_blocking_keep_callbacks],
//!   @[set_nonblocking_keep_callbacks]
{
  // Previously this function wrote the remaining write buffer to the
  // stream directly. But that can only be done safely if we implement
  // a lock here to wait for any nonblocking operations to complete,
  // and that could introduce a deadlock since there might be other
  // lock dependencies between the threads.

  SSL3_DEBUG_MSG ("SSL.File->set_blocking()\n%s",
		  "" || describe_backtrace (backtrace()));

  ENTER (0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    nonblocking_mode = 0;
    accept_callback = read_callback = write_callback = close_callback = 0;

    if (!local_backend) local_backend = Pike.SmallBackend();

    if (stream) {
      stream->set_backend(local_backend);
      stream->set_blocking_keep_callbacks();
    }
  } LEAVE;
}

void set_blocking_keep_callbacks()
//! Set blocking mode like @[set_blocking], but don't alter any
//! callbacks.
//!
//! @seealso
//!   @[set_blocking], @[set_nonblocking]
{
  SSL3_DEBUG_MSG ("SSL.File->set_blocking_keep_callbacks()\n");

  ENTER (0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    nonblocking_mode = 0;

    if (!local_backend) local_backend = Pike.SmallBackend();

    if (stream) {
      stream->set_backend(local_backend);
      stream->set_blocking_keep_callbacks();
    }
  } LEAVE;
}

int errno()
//! @returns
//!   Returns the current error number for the connection.
//!   Notable values are:
//!   @int
//!     @value 0
//!       No error
//!     @value System.EPIPE
//!       Connection closed by other end.
//!   @endint
{
  // We don't check threads for most other query functions, but
  // looking at errno while doing I/O in another thread can't be done
  // safely.
  CHECK (0);
  return local_errno ? local_errno : stream && stream->errno();
}

void set_alert_callback (function(object,int|object,string:void) alert)
//! Install a function that will be called when an alert packet is about
//! to be sent. It doesn't affect the callback mode - it's called both
//! from backends and from within normal function calls like @[read]
//! and @[write].
//!
//! This callback can be used to implement fallback to other protocols
//! when used on the server side together with @[shutdown()].
//!
//! @note
//! This object is part of a cyclic reference whenever this is set,
//! just like setting any other callback.
//!
//! @note
//!   This callback is not cleared by @[set_blocking], or settable
//!   by @[set_callbacks] or @[set_nonblocking]. It is also not
//!   part of the set returned by @[query_callbacks].
//!
//! @seealso
//!   @[query_alert_callback]
{
  SSL3_DEBUG_MSG ("SSL.File->set_alert_callback (%O)\n", alert);
  CHECK (0);
#ifdef SSLFILE_DEBUG
  if (close_state == STREAM_UNINITIALIZED || !conn)
    error ("Doesn't have any connection.\n");
#endif
  conn->set_alert_callback (
    alert &&
    lambda (object packet, int|object seq_num, string alert_context) {
      SSL3_DEBUG_MSG ("Calling alert callback %O\n", alert);
      alert (packet, seq_num, alert_context);
      alert_cb_called = 1;
    });
}

function(object,int|object,string:void) query_alert_callback()
//! @returns
//!   Returns the current alert callback.
//!
//! @seealso
//!   @[set_alert_callback]
{
  return conn && conn->alert_callback;
}

void set_accept_callback (function(void|object,void|mixed:int) accept)
//! Install a function that will be called when the handshake is
//! finished and the connection is ready for use.
//!
//! The callback function will be called with the File object and the
//! additional id arguments (set with @[set_id]).
//! 
//! @note
//! Like the read, write and close callbacks, installing this callback
//! implies callback mode, even after the handshake is done.
//!
//! @seealso
//!   @[set_nonblocking], @[set_callbacks],
//!   @[query_accept_callback], @[query_callbacks]
{
  SSL3_DEBUG_MSG ("SSL.File->set_accept_callback (%O)\n", accept);
  ENTER (0) {
#ifdef SSLFILE_DEBUG
    if (close_state == STREAM_UNINITIALIZED)
      error ("Doesn't have any connection.\n");
#endif
    accept_callback = accept;
    if (accept) schedule_poll();
  } LEAVE;
}

function(void|object,void|mixed:int) query_accept_callback()
//! @returns
//!   Returns the current accept callback.
//!
//! @seealso
//!   @[set_accept_callback]
{
  return accept_callback;
}

void set_read_callback (function(void|mixed,void|string:int) read)
//! Install a function to be called when data is available.
//!
//! @seealso
//!   @[query_read_callback], @[set_nonblocking], @[query_callbacks]
{
  SSL3_DEBUG_MSG ("SSL.File->set_read_callback (%O)\n", read);
  ENTER (0) {
#ifdef SSLFILE_DEBUG
    if (close_state == STREAM_UNINITIALIZED)
      error ("Doesn't have any connection.\n");
#endif
    read_callback = read;
    if (read) schedule_poll();
  } LEAVE;
}

function(void|mixed,void|string:int) query_read_callback()
//! @returns
//!   Returns the current read callback.
//!
//! @seealso
//!   @[set_read_callback], @[set_nonblocking], @[query_callbacks]
{
  return read_callback;
}

void set_write_callback (function(void|mixed:int) write)
//! Install a function to be called when data can be written.
//!
//! @seealso
//!   @[query_write_callback], @[set_nonblocking], @[query_callbacks]
{
  SSL3_DEBUG_MSG ("SSL.File->set_write_callback (%O)\n", write);
  ENTER (0) {
#ifdef SSLFILE_DEBUG
    if (close_state == STREAM_UNINITIALIZED)
      error ("Doesn't have any connection.\n");
#endif
    write_callback = write;
    if (write) schedule_poll();
  } LEAVE;
}

function(void|mixed:int) query_write_callback()
//! @returns
//!   Returns the current write callback.
//!
//! @seealso
//!   @[set_write_callback], @[set_nonblocking], @[query_callbacks]
{
  return write_callback;
}

void set_close_callback (function(void|mixed:int) close)
//! Install a function to be called when the connection is closed,
//! either normally or due to an error (use @[errno] to retrieve it).
//!
//! @seealso
//!   @[query_close_callback], @[set_nonblocking], @[query_callbacks]
{
  SSL3_DEBUG_MSG ("SSL.File->set_close_callback (%O)\n", close);
  ENTER (0) {
#ifdef SSLFILE_DEBUG
    if (close_state == STREAM_UNINITIALIZED)
      error ("Doesn't have any connection.\n");
#endif
    close_callback = close;
    if (close) schedule_poll();
  } LEAVE;
}

function(void|mixed:int) query_close_callback()
//! @returns
//!   Returns the current close callback.
//!
//! @seealso
//!   @[set_close_callback], @[set_nonblocking], @[query_callbacks]
{
  return close_callback;
}

void set_id (mixed id)
//! Set the value to be sent as the first argument to the
//! callbacks installed by @[set_callbacks].
//!
//! @seealso
//!   @[query_id]
{
  SSL3_DEBUG_MSG ("SSL.File->set_id (%O)\n", id);
  CHECK (0);
  callback_id = id;
}

mixed query_id()
//! @returns
//!   Returns the currently set id.
//!
//! @seealso
//!   @[set_id]
{
  return callback_id;
}

void set_backend (Pike.Backend backend)
//! Set the backend used for the file callbacks.
//!
//! @seealso
//!   @[query_backend]
{
  ENTER (0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    if (user_cb_co) {
      real_backend->remove_call_out(user_cb_co);
      user_cb_co = UNDEFINED;
    }

    if (stream) {
      if (nonblocking_mode)
	stream->set_backend (backend);
    }

    real_backend = backend;

    schedule_poll();
  } LEAVE;
}

Pike.Backend query_backend()
//! Return the backend used for the file callbacks.
//!
//! @seealso
//!   @[set_backend]
{
  if (close_state > STREAM_OPEN) error ("Not open.\n");
  return real_backend;
}

string query_address(int|void arg)
//! @returns
//!   Returns the address and port of the connection.
//!
//!   See @[Stdio.File.query_address] for details.
//!
//! @seealso
//!   @[Stdio.File.query_address]
{
  if (close_state > STREAM_OPEN) error ("Not open.\n");
  return stream->query_address(arg);
}

int is_open()
//! @returns
//! Returns nonzero if the stream currently is open, zero otherwise.
//!
//! This function does nonblocking I/O to check for a close packet in
//! the input buffer.
//!
//! If a clean close has been requested in nonblocking mode, then 2 is
//! returned until the close packet exchanged has been completed.
//!
//! @note
//! In Pike 7.8 and earlier, this function returned zero in the case
//! above where it now returns 2.
{
  SSL3_DEBUG_MSG ("SSL.File->is_open()\n");
  ENTER (0) {
    if ((close_state == STREAM_OPEN || close_state == CLEAN_CLOSE) &&
	stream && stream->is_open()) {
      // When close_state == STREAM_OPEN, we have to check if there's
      // a close packet waiting to be read. This is common in
      // keep-alive situations since the remote end might have sent a
      // close packet and closed the connection a long time ago, and
      // in a typical blocking client no action has been taken causing
      // the close packet to be read.
      //
      // If close_state == CLEAN_CLOSE, we should return true as long
      // as close packets haven't been exchanged in both directions.
      //
      // Could avoid the whole local backend hoopla here by
      // essentially doing a peek and call ssl_read_callback directly,
      // but that'd lead to subtle code duplication. (Also, peek is
      // currently not implemented on NT.)
      ConnectionState closed = conn->state & CONNECTION_closed;
      if ((close_state == CLEAN_CLOSE ?
	   closed != CONNECTION_closed : !closed))
	RUN_MAYBE_BLOCKING (
	  action && (close_state == CLEAN_CLOSE ?
		     (conn->state & CONNECTION_closed) != CONNECTION_closed :
		     !(conn->state & CONNECTION_closed)),
	  1);
      closed = conn->state & CONNECTION_closed;
      RETURN (close_state == CLEAN_CLOSE ?
	      ((closed != CONNECTION_closed) && 2) : !closed);
    }
  } LEAVE;
  return 0;
}

Stdio.File query_stream()
//! Return the underlying stream.
//!
//! @note
//! Avoid any temptation to do
//! @expr{destruct(file_obj->query_stream())@}. That almost certainly
//! creates more problems than it solves.
//!
//! You probably want to use @[shutdown].
//!
//! @seealso
//!   @[shutdown]
{
  SSL3_DEBUG_MSG ("SSL.File->query_stream(): Called from %s:%d\n",
		  backtrace()[-2][0], backtrace()[-2][1]);
  return stream;
}

.Connection query_connection()
//! Return the SSL connection object.
//!
//! This returns the low-level @[SSL.connection] object.
{
  SSL3_DEBUG_MSG ("SSL.File->query_connection(): Called from %s:%d\n",
		  backtrace()[-2][0], backtrace()[-2][1]);
  return conn;
}

SSL.Context query_context()
//! Return the SSL context object.
{
  return conn && conn->context;
}

protected string _sprintf(int t)
{
  return t=='O' && sprintf("SSL.File(%O, %O)",
			   stream && stream->_fd, conn);
}


protected int queue_write()
// Return 0 if the connection is still alive, 1 if it was closed
// politely, and -1 if it died unexpectedly (specifically, our side
// has sent a fatal alert packet (not close notify) for some reason
// and therefore nullified the connection).
{
  if (!conn) return -1;

  // Estimate how much data there is in the write_buffer.
  int got = sizeof(write_buffer) &&
    (sizeof(write_buffer[-1]) * sizeof(write_buffer));

  int buffer_limit = 16384;
  if (conn->state & CONNECTION_closing) buffer_limit = 1;

  int|string res;
  while (got < buffer_limit) {
    res = conn->to_write();

#ifdef SSL3_DEBUG_TRANSPORT
    werror ("queue_write: To write: %O\n", res);
#endif

    if (!stringp(res)) {
      SSL3_DEBUG_MSG ("queue_write: Connection closed %s\n",
		      res == 1 ? "normally" : "abruptly");
      break;
    }

    if (res == "") {
      SSL3_DEBUG_MSG ("queue_write: Got nothing to write (%d strings buffered)\n",
		      sizeof (write_buffer));
      res = 0;
      break;
    }

    int was_empty = !sizeof (write_buffer);
    write_buffer += ({ res });
    got += sizeof(res);

    SSL3_DEBUG_MSG ("queue_write: Got %d bytes to write (%d strings buffered)\n",
		    sizeof (res), sizeof (write_buffer));
    res = 0;
  }

  if (!sizeof(write_buffer)) {
    if (stream) stream->set_write_callback(0);
    if (!(conn->state & CONNECTION_handshaking)) {
      SSL3_DEBUG_MSG("queue_write: Write buffer empty -- ask for some more data.\n");
      schedule_poll();
    }
  } else if (stream) {
    SSL3_DEBUG_MSG("queue_write: Install the write callback.\n");
    stream->set_write_callback(ssl_write_callback);
  }

  SSL3_DEBUG_MSG ("queue_write: Returning %O (%d strings buffered)\n",
		  res, sizeof(write_buffer));

  return res;
}

protected int direct_write()
// Do a write directly (and maybe also read if there's internal
// reading to be done). Something to write is assumed to exist (either
// in write_buffer or in the packet queue). Returns zero on error (as
// opposed to queue_write).
{
  if (!stream) {
    SSL3_DEBUG_MSG ("direct_write: "
		    "Connection already closed - simulating System.EPIPE\n");
    // If it was closed explicitly locally then close_state would be
    // set and we'd never get here, so we can report it as a remote
    // close.
    cleanup_on_error();
    local_errno = System.EPIPE;
    return 0;
  }

  else {
    if (queue_write() < 0 || close_state == ABRUPT_CLOSE) {
      SSL3_DEBUG_MSG ("direct_write: "
		      "Connection closed abruptly - simulating System.EPIPE\n");
      cleanup_on_error();
      local_errno = System.EPIPE;
      // Don't set close_state = ABRUPT_CLOSE here. It's either set
      // already, or there's an abrupt close due to an ALERT_fatal,
      // which we shouldn't mix up with ABRUPT_CLOSE.
      return 0;
    }

    if (SSL_INTERNAL_WRITING || SSL_INTERNAL_READING)
      RUN_MAYBE_BLOCKING (SSL_INTERNAL_WRITING || SSL_INTERNAL_READING,
			  nonblocking_mode);
  }

  SSL3_DEBUG_MORE_MSG ("direct_write: Ok\n");
  return 1;
}

protected int ssl_read_callback (int ignored, string input)
{
  SSL3_DEBUG_MSG ("ssl_read_callback (%s): "
		  "nonblocking mode=%d, callback mode=%d %s%s\n",
		  input ? "string[" + sizeof (input) + "]" : "0 (queued extra call)",
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn ? conn->describe_state() : "not connected",
		  conn && SSL_CLOSING_OR_CLOSED ?
		  ", closing (" + close_state + ")" : "");

  ENTER (1) {
    if (input) {
      int handshake_already_finished = !(conn->state & CONNECTION_handshaking);
      string|int data =
	close_state == ABRUPT_CLOSE ? -1 : conn->got_data (input);

#ifdef SSL3_DEBUG_TRANSPORT
      werror ("ssl_read_callback: Got data: %O\n", data);
#endif

      if (alert_cb_called)
	SSL3_DEBUG_MSG ("ssl_read_callback: After alert cb call\n");

      else {
	int write_res;
	if (stringp(data) || (data > 0) ||
	    ((data < 0) && (conn->state & CONNECTION_handshaking))) {
#ifdef SSLFILE_DEBUG
	  if (!stream)
	    error ("Got zapped stream in callback.\n");
#endif
	  // got_data might have put more packets in the write queue if
	  // we're handshaking.
	  write_res = queue_write();
	}

	read_errno = 0;

	if (stringp (data)) {
	  if (!handshake_already_finished &&
	      !(conn->state & CONNECTION_handshaking)) {
	    SSL3_DEBUG_MSG ("ssl_read_callback: Handshake finished\n");
	  }

	  SSL3_DEBUG_MSG ("ssl_read_callback: "
			  "Got %d bytes of application data\n", sizeof (data));
	  read_buffer->add (data);
	  if (sizeof(read_buffer) >= read_buffer_threshold) {
	    SSL3_DEBUG_MSG("SSL.File->rcb: Removing read_callback.\n");
	    stream->set_read_callback(0);
	  }
	}

	else if (data < 0 || write_res < 0) {
	  SSL3_DEBUG_MSG ("ssl_read_callback: "
			  "Got abrupt remote close - simulating System.EPIPE\n");
	  cleanup_on_error();
	  read_errno = System.EPIPE;
	}

	// Don't use data > 0 here since we might have processed some
	// application data and a close in the same got_data call.
	if (conn->state & CONNECTION_peer_closed) {
	  SSL3_DEBUG_MSG ("ssl_read_callback: Got close packet\n");
	}
      }
    }

    // Figure out what we need to do. call_accept_cb is already set
    // from above.
    if (alert_cb_called) {
      if (!conn) {
	SSL3_DEBUG_MSG ("ssl_read_callback: Shut down from alert callback\n");
	RESTORE;
	return 0;
      }
      // Make sure the alert is queued for writing.
      queue_write();
    }

    if (!conn || (conn->state & CONNECTION_peer_closed)) {
      // Deinstall read side cbs to avoid reading more.
      SSL3_DEBUG_MSG("SSL.File->direct_write: Removing read/close_callback.\n");
      stream->set_read_callback (0);
      stream->set_close_callback (0);
    }

    // Now actually do (a bit of) what should be done.
    schedule_poll();

  } LEAVE;
  return 0;
}

protected int ssl_write_callback (int ignored)
{
  SSL3_DEBUG_MSG ("ssl_write_callback: "
		  "nonblocking mode=%d, callback mode=%d %s%s\n",
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn ? conn->describe_state() : "",
		  conn && SSL_CLOSING_OR_CLOSED ?
		  ", closing (" + close_state + ")" : "");

  int ret = 0;

  ENTER (1) {
#ifdef SSLFILE_DEBUG
    if (!stream)
      error ("Got zapped stream in callback.\n");
#endif

  write_to_stream:
    do {
      if (sizeof (write_buffer)) {
	int written;
#ifdef SIMULATE_CLOSE_PACKET_WRITE_FAILURE
	if (conn->state & CONNECTION_local_closing)
	  written = -1;
	else
#endif
	  written = stream->write (write_buffer);

	if (written < 0 ) {
#ifdef SIMULATE_CLOSE_PACKET_WRITE_FAILURE
	  if (conn->state & CONNECTION_local_closing)
	    write_errno = System.EPIPE;
	  else
#endif
	    write_errno = stream->errno();
	  cleanup_on_error();
	  SSL3_DEBUG_MSG ("ssl_write_callback: Write failed: %s\n",
			  strerror (write_errno));

	  // Make sure the local backend exits after this, so that the
	  // error isn't clobbered by later I/O.
	  ret = -1;

	  if (conn->state & CONNECTION_local_closing &&
	      epipe_errnos[write_errno]) {
	    // See if it's an error from writing a close packet that
	    // should be ignored.
	    write_buffer = ({}); // No use trying to write the close again.
	    close_errno = write_errno;

	    if (close_state == CLEAN_CLOSE) {
	      // Never accept a failure if a clean close is requested.
	      if (!(conn->state & CONNECTION_failing)) {
		// Make sure that the connection is failing.
		conn->send_packet(conn->alert(ALERT_fatal, ALERT_close_notify));
	      }
	    }

	    else {
	      close_errno = 0;

	      if (conn->state & CONNECTION_peer_closed)
		SSL3_DEBUG_MSG ("ssl_write_callback: Stream closed properly "
				"remotely - ignoring failure to send close packet\n");

	      else {
		SSL3_DEBUG_MSG ("ssl_write_callback: Stream closed remotely - "
				"checking input buffer for proper remote close\n");
		RESTORE;
		return ret;
	      }
	    }
	  }

	  // Still try to call the write (or close) callback to
	  // propagate the error to it.
	  break write_to_stream;
	}

	for (int bytes = written; bytes > 0;) {
	  if (bytes >= sizeof(write_buffer[0])) {
	    bytes -= sizeof(write_buffer[0]);
	    write_buffer = write_buffer[1..];
	  } else {
	    write_buffer[0] = write_buffer[0][bytes..];
	    bytes = 0;
	    break;
	  }
	}

	SSL3_DEBUG_MSG ("ssl_write_callback: Wrote %d bytes (%d strings left)\n",
			written, sizeof (write_buffer));
	if (sizeof (write_buffer)) {
	  RESTORE;
	  return ret;
	}
      }

      if (int err = queue_write()) {
	if (err > 0) {
#ifdef SSLFILE_DEBUG
	  if (!(conn->state & CONNECTION_closed))
	    error ("Expected a close to be sent or received\n");
#endif

	  if (sizeof (write_buffer))
	    SSL3_DEBUG_MSG ("ssl_write_callback: "
			    "Close packet queued but not yet sent\n");
	  else {
#ifdef SSLFILE_DEBUG
	    if (!(conn->state & CONNECTION_local_closed))
	      error ("Expected a close packet to be queued or sent.\n");
#endif
	    if (conn->state & CONNECTION_peer_closed ||
		close_state != CLEAN_CLOSE) {
	      SSL3_DEBUG_MSG ("ssl_write_callback: %s\n",
			      conn->state & CONNECTION_peer_closed ?
			      "Close packets exchanged" : "Close packet sent");
	      break write_to_stream;
	    }
	    else {
	      SSL3_DEBUG_MSG ("ssl_write_callback: "
			      "Close packet sent - expecting response\n");
	      stream->set_read_callback(ssl_read_callback);
	      stream->set_close_callback(ssl_close_callback);
	      schedule_poll();
	    }
	  }

	  RESTORE;
	  return ret;
	}

	else {
	  SSL3_DEBUG_MSG ("ssl_write_callback: "
			  "Connection closed abruptly remotely - "
			  "simulating System.EPIPE\n");
	  cleanup_on_error();
	  write_errno = System.EPIPE;
	  ret = -1;
	  break write_to_stream;
	}
      }
    } while (sizeof (write_buffer));

    schedule_poll();

    if ((close_state >= NORMAL_CLOSE &&
	 (conn->state & CONNECTION_local_closing)) || write_errno) {
      SSL3_DEBUG_MSG ("ssl_write_callback: "
		      "In or after local close - shutting down\n");
      shutdown();
    }
  } LEAVE;
  return ret;
}

protected int ssl_close_callback (int ignored)
{
  SSL3_DEBUG_MSG ("ssl_close_callback: "
		  "nonblocking mode=%d, callback mode=%d %s%s\n",
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn ? conn->describe_state() : "",
		  conn && SSL_CLOSING_OR_CLOSED ?
		  ", closing (" + close_state + ")" : "");

  ENTER (1) {
#ifdef SSLFILE_DEBUG
    if (!stream)
      error ("Got zapped stream in callback.\n");
#endif

    // If we've arrived here due to an error, let it override any
    // older errno from an earlier callback.
    if (int new_errno = stream->errno()) {
      SSL3_DEBUG_MSG ("ssl_close_callback: Got error %s\n", strerror (new_errno));
      cleanup_on_error();
      close_errno = new_errno;
    }
#ifdef SSL3_DEBUG
    else if (close_errno)
      SSL3_DEBUG_MSG ("ssl_close_callback: Propagating errno from another callback\n");
#endif

    if (!close_errno) {
      if (!conn || conn->state & CONNECTION_peer_closed)
	SSL3_DEBUG_MSG ("ssl_close_callback: After clean close\n");

      else {
	// The remote end has closed the connection without sending a
	// close packet.
	//
	// This became legal by popular demand in TLS 1.1.
	SSL3_DEBUG_MSG ("ssl_close_callback: Did not get a remote close.\n");
	conn->state = [int(0..0)|ConnectionState]
	  (conn->state | CONNECTION_peer_closed);
	SSL3_DEBUG_MSG ("ssl_close_callback: Abrupt close - "
			"simulating System.EPIPE\n");
	cleanup_on_error();
	close_errno = System.EPIPE;
	close_state = ABRUPT_CLOSE;
      }
    }

    schedule_poll();

    if (close_state >= NORMAL_CLOSE) {
      SSL3_DEBUG_MSG ("ssl_close_callback: "
		      "In or after local close - shutting down\n");
      shutdown();
    }
  } LEAVE;

  // Make sure the local backend exits after this, so that the error
  // isn't clobbered by later I/O.
  return -1;
}

//! The application protocol chosen by the client during application layer
//! protocol negotiation (ALPN).
string `->application_protocol() {
    return conn->application_protocol;
}

//! Return the currently active cipher suite.
int query_suite()
{
  return conn?->session?->cipher_suite;
}
