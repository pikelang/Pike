#pike 7.8

#pragma no_deprecation_warnings

#if constant(SSL.Cipher.CipherAlgorithm)

//! Interface similar to @[Stdio.File].
//!
//! @ul
//! @item
//!   Handles blocking and nonblocking mode.
//! @item
//!   Handles callback mode in an arbitrary backend (also in blocking
//!   mode).
//! @item
//!   Read and write operations might each do both reading and
//!   writing. In callback mode that means that installing either a
//!   read or a write callback might install both internally. It also
//!   means that reading in one thread while writing in another
//!   doesn't work.
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

// #define DEBUG
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

protected SSL.connection conn;
// Always set when stream is. Destructed with destroy() at shutdown
// since it contains cyclic references. Noone else gets to it, though.

protected array(string) write_buffer; // Encrypted data to be written.
protected String.Buffer read_buffer; // Decrypted data that has been read.

protected mixed callback_id;
protected function(void|object,void|mixed:int) accept_callback;
protected function(void|mixed,void|string:int) read_callback;
protected function(void|mixed:int) write_callback;
protected function(void|mixed:int) close_callback;

protected Pike.Backend real_backend;
// The real backend for the stream.

protected Pike.Backend local_backend;
// Internally all I/O is done using callbacks. When the real backend
// can't be used, either because we aren't in callback mode (i.e. the
// user hasn't registered any callbacks), or because we're to do a
// blocking operation, this local one takes its place. It's
// created on demand.

protected int nonblocking_mode;

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

protected enum ClosePacketSendState {
  CLOSE_PACKET_NOT_SCHEDULED = 0,
  CLOSE_PACKET_SCHEDULED,
  CLOSE_PACKET_QUEUED_OR_DONE,
  CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR,
  CLOSE_PACKET_WRITE_ERROR,
}
protected ClosePacketSendState close_packet_send_state;
// State for the close packet we send. The trickiness here is that if
// there's an error writing it, that error should sometimes be
// ignored:
//
// The remote end is allowed to close the connection immediately
// without waiting for a response, and we can't close without
// attempting to send a close packet first (RFC 2246, section 7.2.1).
// So if we've received a close packet we need to try to send one but
// ignore any error (typically EPIPE).
//
// Also, if there is an error we can't report it immediately since the
// remote close packet might be sitting in the input buffer.

protected int local_errno;
// If nonzero, override the errno on the stream with this.

protected int cb_errno;
// Stores the errno from failed I/O in a callback so that the next
// visible I/O operation can report it properly.

protected int got_extra_read_call_out;
// 1 when we have a call out to ssl_read_callback. We get this when we
// need to call read_callback or close_callback but can't do that
// right away from ssl_read_callback. See comments in that function
// for more details. -1 if we've switched to non-callback mode and
// therefore has removed the call out temporarily but need to restore
// it when switching back. 0 otherwise.
//
// -1 is also set before a call to update_internal_state when we want
// to schedule an extra read call out; update_internal_state will then
// do the actual call out installation if possible.

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

// This macro is used in all user called functions that can report I/O
// errors, both at the beginning and after
// ssl_(read|write|close)_callback calls.
#define FIX_ERRNOS(ERROR, NO_ERROR) do {				\
    if (cb_errno) {							\
      /* Got a stored error from a previous callback that has failed. */ \
      local_errno = cb_errno;						\
      cb_errno = 0;							\
      {ERROR;}								\
    }									\
    else {								\
      local_errno = 0;							\
      {NO_ERROR;}							\
    }									\
  } while (0)

// Ignore user installed callbacks if a close has been requested locally.
#define CALLBACK_MODE							\
  ((read_callback || write_callback || close_callback || accept_callback) && \
   close_state < NORMAL_CLOSE)

#define SSL_HANDSHAKING (!conn->handshake_finished &&			\
			 close_state != ABRUPT_CLOSE)
#define SSL_CLOSING_OR_CLOSED						\
  (close_packet_send_state >= CLOSE_PACKET_SCHEDULED ||			\
   /* conn->closing & 2 is more accurate, but the following is quicker	\
    * and only overlaps a bit with the check above. */			\
   conn->closing)

// Always wait for input during handshaking and when we expect the
// remote end to respond to our close packet. We should also check the
// input buffer for a close packet if there was a failure to write our
// close packet.
#define SSL_INTERNAL_READING						\
  (SSL_HANDSHAKING ||							\
   (close_state == CLEAN_CLOSE ?					\
    conn->closing == 1 :						\
    close_packet_send_state == CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR))

// Try to write when there's data in the write buffer or when we have
// a close packet to send. The packet is queued separately by
// ssl_write_callback in the latter case.
#define SSL_INTERNAL_WRITING (sizeof (write_buffer) ||			\
			      close_packet_send_state == CLOSE_PACKET_SCHEDULED)

#ifdef DEBUG

#if constant (Thread.thread_create)

#define THREAD_T Thread.Thread
#define THIS_THREAD() this_thread()


protected void thread_error (string msg, THREAD_T other_thread)
{
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

protected THREAD_T op_thread;

// FIXME: Looks like the following check can give false alarms since
// an fd object can lose all refs even if some callbacks still are
// registered.
#define CHECK_CB_MODE(CUR_THREAD) do {					\
    if (Pike.Backend backend = stream && stream->query_backend()) {	\
      THREAD_T backend_thread = backend->executing_thread();		\
      if (backend_thread && backend_thread != CUR_THREAD &&		\
	  (stream->query_read_callback() ||				\
	   stream->query_write_callback() ||				\
	   stream->query_close_callback()))				\
	/* NB: The other thread backtrace might not be relevant at	\
	 * all here. */							\
	thread_error ("In callback mode in a different backend.\n",	\
		      backend_thread);					\
    }									\
  } while (0)

#define LOW_CHECK(OP_THREAD, CUR_THREAD,				\
		  IN_CALLBACK, CALLED_FROM_REAL_BACKEND) do {		\
    if (IN_CALLBACK) {							\
      if (CALLED_FROM_REAL_BACKEND) {					\
	if (OP_THREAD)							\
	  thread_error ("Called from real backend while doing an operation.\n",	\
			OP_THREAD);					\
      }									\
      else								\
	if (!OP_THREAD)							\
	  error ("Called from local backend outside an operation.\n");	\
    }									\
									\
    if (OP_THREAD && OP_THREAD != CUR_THREAD)				\
      thread_error ("Doing operation in another thread.\n", OP_THREAD);	\
									\
    CHECK_CB_MODE (CUR_THREAD);						\
  } while (0)

#define CHECK(IN_CALLBACK, CALLED_FROM_REAL_BACKEND) do {		\
    THREAD_T cur_thread = THIS_THREAD();				\
    LOW_CHECK (op_thread, cur_thread, IN_CALLBACK, CALLED_FROM_REAL_BACKEND); \
  } while (0)

#define ENTER(IN_CALLBACK, CALLED_FROM_REAL_BACKEND) do {		\
    THREAD_T old_op_thread;						\
    {									\
      THREAD_T cur_thread = THIS_THREAD();				\
      old_op_thread = op_thread;					\
      /* Relying on the interpreter lock here. */			\
      op_thread = cur_thread;						\
    }									\
    LOW_CHECK (old_op_thread, op_thread, IN_CALLBACK, CALLED_FROM_REAL_BACKEND); \
    mixed _op_err = catch

#define RESTORE do {op_thread = old_op_thread;} while (0)

#define RETURN(RET_VAL) do {RESTORE; return (RET_VAL);} while (0)

#define LEAVE								\
  ;									\
  RESTORE;								\
  if (_op_err) throw (_op_err);						\
  } while (0)

#else  // !DEBUG

#define CHECK_CB_MODE(CUR_THREAD) do {} while (0)
#define CHECK(IN_CALLBACK, CALLED_FROM_REAL_BACKEND) do {} while (0)
#define ENTER(IN_CALLBACK, CALLED_FROM_REAL_BACKEND) do
#define RESTORE do {} while (0)
#define RETURN(RET_VAL) return (RET_VAL)
#define LEAVE while (0)

#endif	// !DEBUG

// stream is assumed to be operational on entry but might be zero
// afterwards. cb_errno is assumed to be 0 on entry.
#define RUN_MAYBE_BLOCKING(REPEAT_COND, NONWAITING_MODE,		\
			   ENABLE_READS, ERROR_CODE) do {		\
  run_local_backend: {							\
      CHECK_CB_MODE (THIS_THREAD());					\
      if (!local_backend) local_backend = Pike.SmallBackend();		\
      stream->set_backend (local_backend);				\
      stream->set_id (0);						\
									\
      while (1) {							\
	float|int(0..0) action;						\
									\
	if (got_extra_read_call_out) {					\
	  /* Do whatever ssl_read_callback needs to do before we	\
	   * continue. Since the first arg is zero here it won't call	\
	   * any user callbacks, so they are superseded as they should	\
	   * be if we're doing an explicit read (or a write or close,	\
	   * which legitimately might cause reading to be done). Don't	\
	   * need to bother with the return value from ssl_read_callback \
	   * since we'll propagate the error, if any, just below. */	\
	  if (got_extra_read_call_out > 0)				\
	    real_backend->remove_call_out (ssl_read_callback);		\
	  ssl_read_callback (0, 0); /* Will clear got_extra_read_call_out. */ \
	  action = 0.0;							\
	}								\
									\
	else {								\
	  stream->set_write_callback (SSL_INTERNAL_WRITING && ssl_write_callback); \
									\
	  if (ENABLE_READS) {						\
	    stream->set_read_callback (ssl_read_callback);		\
	    stream->set_close_callback (ssl_close_callback);		\
	  }								\
	  else {							\
	    stream->set_read_callback (0);				\
	    /* Installing a close callback without a read callback	\
	     * currently doesn't work well in Stdio.File. */		\
	    stream->set_close_callback (0);				\
	  }								\
									\
	  /* When we fail to write the close packet we should check if	\
	   * a close packet is in the input buffer before signalling	\
	   * the error. That means installing the read callbacks	\
	   * without waiting in the backend. */				\
	  int zero_timeout = NONWAITING_MODE ||				\
	    close_packet_send_state == CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR; \
									\
	  SSL3_DEBUG_MSG ("Running local backend [r:%O w:%O], %s timeout\n", \
			  !!stream->query_read_callback(),		\
			  !!stream->query_write_callback(),		\
			  zero_timeout ? "zero" : "infinite");		\
									\
	  action = local_backend (zero_timeout ? 0.0 : 0);		\
	}								\
									\
	if (!action &&							\
	    close_packet_send_state == CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR) { \
	  SSL3_DEBUG_MSG ("Did not get a remote close - "		\
			  "signalling delayed error from writing close message\n"); \
	  cleanup_on_error();						\
	  close_packet_send_state = CLOSE_PACKET_WRITE_ERROR;		\
	  cb_errno = System.EPIPE;					\
	  close_state = ABRUPT_CLOSE;					\
	}								\
									\
	FIX_ERRNOS ({							\
	    SSL3_DEBUG_MSG ("Local backend ended with error\n");	\
	    if (stream) {						\
	      stream->set_id (1);					\
	      update_internal_state (1);				\
	      /* Switch backend after updating the installed callbacks. */ \
	      stream->set_backend (real_backend);			\
	      CHECK_CB_MODE (THIS_THREAD());				\
	    }								\
	    {ERROR_CODE;}						\
	    break run_local_backend;					\
	  }, 0);							\
									\
	if (!stream) {							\
	  SSL3_DEBUG_MSG ("Local backend ended after close\n");		\
	  break run_local_backend;					\
	}								\
									\
	if (NONWAITING_MODE && !action) {				\
	  SSL3_DEBUG_MSG ("Nonwaiting local backend ended - nothing to do\n"); \
	  break;							\
	}								\
									\
	if (!(REPEAT_COND)) {						\
	  SSL3_DEBUG_MSG ("Local backend ended - repeat condition false\n"); \
	  break;							\
	}								\
      }									\
									\
      stream->set_id (1);						\
      update_internal_state (1);					\
      /* Switch backend after updating the installed callbacks. */	\
      stream->set_backend (real_backend);				\
      CHECK_CB_MODE (THIS_THREAD());					\
    }									\
  } while (0)

protected void create (Stdio.File stream, SSL.context ctx,
		       int|void is_client, int|void is_blocking,
		       SSL.Constants.ProtocolVersion|void min_version,
		       SSL.Constants.ProtocolVersion|void max_version)
//! Create an SSL connection over an open @[stream].
//!
//! @param stream
//!   Open socket or pipe to create the connection over.
//!
//! @param ctx
//!   The SSL context.
//!
//! @param is_client
//!   If is set then a client-side connection is started,
//!   server-side otherwise.
//!
//! @param is_blocking
//!   If is set then the stream is initially set in blocking
//!   mode, nonblocking mode otherwise.
//!
//! @param min_version
//!   The minimum minor version of SSL to support.
//!   Defaults to @[PROTOCOL_SSL_3_0].
//!
//! @param max_version
//!   The maximum minor version of SSL to support.
//!   Defaults to @[PROTOCOL_minor].
//!
//! The backend used by @[stream] is taken over and restored after the
//! connection is closed (see @[close] and @[shutdown]). The callbacks
//! and id in @[stream] are overwritten.
//!
//! @throws
//!   Throws errors on handshake failure in blocking client mode.
{
  SSL3_DEBUG_MSG ("SSL.sslfile->create (%O, %O, %O, %O, %O, %O)\n",
		  stream, ctx, is_client, is_blocking,
		  min_version, max_version);

  ENTER (0, 0) {
    global::stream = stream;
#ifdef SSL3_DEBUG
    if (stream->query_fd)
      stream_descr = "fd:" + stream->query_fd();
    else
      stream_descr = replace (sprintf ("%O", stream), "%", "%%");
#endif
    write_buffer = ({});
    read_buffer = String.Buffer();
    real_backend = stream->query_backend();
    close_state = STREAM_OPEN;

    stream->set_read_callback (0);
    stream->set_write_callback (0);
    stream->set_close_callback (0);
    stream->set_id (1);

    if(!ctx->random)
      ctx->random = Crypto.Random.random_string;
    conn = SSL.connection (!is_client, ctx, min_version, max_version);

    if(is_blocking) {
      set_blocking();
      if (is_client && !direct_write()) {
	int err = errno();
	shutdown();
	local_errno = err;
	error("SSL.sslfile: SSL handshake failure: %s\n", strerror(err));
      }
    }
    else {
      set_nonblocking();
      if (is_client) queue_write();
    }
  } LEAVE;
}

mixed get_server_names()
{
  return conn->server_names;
}

//! @returns
//!   Returns peer certificate information, if any.
mapping get_peer_certificate_info()
{
  return conn->session->cert_data;
}

//! @returns
//!   Returns the peer certificate chain, if any.
array get_peer_certificates()
{
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
  SSL3_DEBUG_MSG ("SSL.sslfile->close (%O, %O, %O)\n",
		  how, clean_close, dont_throw);

  if (how && how != "rw")
    error ("Can only close the connection in both directions simultaneously.\n");

  ENTER (0, 0) {
    if (close_state > STREAM_OPEN) {
      SSL3_DEBUG_MSG ("SSL.sslfile->close: Already closed (%d)\n", close_state);
      RETURN (1);
    }
    close_state = clean_close ? CLEAN_CLOSE : NORMAL_CLOSE;

    FIX_ERRNOS ({
	SSL3_DEBUG_MSG ("SSL.sslfile->close: Shutdown after error\n");
	int err = errno();
	shutdown();
	// Get here e.g. if a close callback calls close after an
	// error, so never throw. (I'm somewhat suspicious to this,
	// but I guess I did it with good reason.. :P /mast)
	local_errno = err;
	RETURN (0);
      }, 0);

    if (dont_throw) {
      close_packet_send_state = CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR;
    } else if (close_packet_send_state == CLOSE_PACKET_NOT_SCHEDULED)
      close_packet_send_state = CLOSE_PACKET_SCHEDULED;

    // Even in nonblocking mode we call direct_write here to try to
    // put the close packet in the send buffer before we return. That
    // way it has a fair chance to get sent even when we're called
    // from destroy() (in which case it won't work to just install the
    // write callback as usual and wait for the backend to call it).

    if (!direct_write()) {
      // Should be shut down after close(), even if an error occurred.
      int err = errno();
      shutdown();
      local_errno = err;
      if (dont_throw) {
	RETURN (0);
      }
      else if (!(< 0, System.EPIPE, System.ECONNRESET, >)[err]) {
	// Errors are normally thrown from close().
        error ("Failed to close SSL connection: %s\n", strerror (err));
      }
    }

    if (stream && (stream->query_read_callback() || stream->query_write_callback())) {
      SSL3_DEBUG_MSG ("SSL.sslfile->close: Close underway\n");
      RESTORE;
      update_internal_state();
      return 1;
    }
    else {
      // The local backend run by direct_write has typically already
      // done this, but it might happen that it doesn't do anything in
      // case close packets already have been exchanged.
      shutdown();
      SSL3_DEBUG_MSG ("SSL.sslfile->close: Close done\n");
    }
  } LEAVE;
  return 1;
}

protected void cleanup_on_error()
// Called when any error occurs on the stream. (Doesn't handle errno
// reporting since it might involve either local_errno and/or
// cb_errno.)
{
  // The session should be purged when an error has occurred.
  if (conn->session)
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
  ENTER (0, 0) {
    if (!stream) {
      SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): Already shut down\n");
      RETURN (0);
    }

    if (close_state == STREAM_OPEN || close_state == NORMAL_CLOSE)
      // If we didn't request a clean close then we pretend to have
      // received a close message. According to the standard it's ok
      // anyway as long as the transport isn't used for anything else.
      conn->closing |= 2;

    SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): %s\n",
		    close_state != ABRUPT_CLOSE &&
		    conn->closing & 2 &&
		    close_packet_send_state == CLOSE_PACKET_QUEUED_OR_DONE &&
		    !sizeof (write_buffer) ?
		    "Proper close" :
		    close_state == STREAM_OPEN ?
		    "Not closed" :
		    "Abrupt close");

    if ((conn->closing & 2) && sizeof (conn->left_over || "")) {
#ifdef DEBUG
      werror ("Warning: Got buffered data after close in %O: %O%s\n", this,
	      conn->left_over[..99], sizeof (conn->left_over) > 100 ? "..." : "");
#endif
      read_buffer = String.Buffer (sizeof (conn->left_over));
      read_buffer->add (conn->left_over);
      close_state = STREAM_OPEN;
    }

    int conn_closing = conn->closing;
    destruct (conn);		// Necessary to avoid garbage.

    write_buffer = ({});

    if (got_extra_read_call_out > 0)
      real_backend->remove_call_out (ssl_read_callback);
    got_extra_read_call_out = 0;

    Stdio.File stream = global::stream;
    global::stream = 0;

    stream->set_read_callback (0);
    stream->set_write_callback (0);
    stream->set_close_callback (0);

    switch (close_state) {
      case CLEAN_CLOSE:
	if (conn_closing == 3) {
	  SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): Clean close - "
			  "leaving stream\n");
	  local_errno = 0;
	  RETURN (stream);
	}
	else {
	  SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): Close packets not fully "
			  "exchanged after clean close (%d) - closing stream\n",
			  conn_closing);
	  stream->close();
	  local_errno = System.EPIPE;
	  RETURN (0);
	}
      case STREAM_OPEN:
	close_state = STREAM_UNINITIALIZED;
	SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): Not closed - leaving stream\n");
	local_errno = 0;
	RETURN (stream);
      default:
	SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): Nonclean close - closing stream\n");
	// if (stream->linger) stream->linger(0);
	stream->close();
	local_errno = stream->errno();
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
  SSL3_DEBUG_MSG ("SSL.sslfile->destroy()\n");

  // We don't know which thread this will be called in if the refcount
  // garb or the gc got here. That's not a race problem since it won't
  // be registered in a backend in that case.
  if (stream) {
    // Make sure not to fail in ENTER below due to bad backend thread.
    // [bug 6958].
    stream->set_callbacks(0, 0, 0);
  }
  ENTER (0, 0) {
    if (stream) {
      if (close_state == STREAM_OPEN &&
	  // Don't bother with closing nicely if there's an error from
	  // an earlier operation. close() will throw an error for it.
	  !cb_errno) {
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
  SSL3_DEBUG_MSG ("SSL.sslfile->read (%d, %d)\n", length, not_all);

  ENTER (0, 0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    FIX_ERRNOS ({
	SSL3_DEBUG_MSG ("SSL.sslfile->read: Propagating old callback error: %s\n",
			strerror (local_errno));
	RETURN (0);
      }, 0);

    if (stream)
      if (not_all) {
	if (!sizeof (read_buffer))
	  RUN_MAYBE_BLOCKING (!sizeof (read_buffer) && !(conn->closing & 2), 0, 1,
			      if (sizeof (read_buffer)) {
				// Got data to return first. Push the
				// error back so it'll get reported by
				// the next call.
				cb_errno = local_errno;
				local_errno = 0;
			      }
			      else RETURN (0););
      }
      else {
	if (sizeof (read_buffer) < length || zero_type (length))
	  RUN_MAYBE_BLOCKING ((sizeof (read_buffer) < length || zero_type (length)) &&
			      !(conn->closing & 2),
			      nonblocking_mode, 1,
			      if (sizeof (read_buffer)) {
				// Got data to return first. Push the
				// error back so it'll get reported by
				// the next call.
				cb_errno = local_errno;
				local_errno = 0;
			      }
			      else RETURN (0););
      }

    string res = read_buffer->get();
    if (!zero_type (length)) {
      read_buffer->add (res[length..]);
      res = res[..length-1];
    }

    SSL3_DEBUG_MSG ("SSL.sslfile->read: Read done, returning %d bytes "
		    "(%d still in buffer)\n",
		    sizeof (res), sizeof (read_buffer));
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
  if (sizeof (args))
    data = sprintf (arrayp (data) ? data * "" : data, @args);

  SSL3_DEBUG_MSG ("SSL.sslfile->write (%t[%d])\n", data, sizeof (data));

  ENTER (0, 0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    FIX_ERRNOS ({
	SSL3_DEBUG_MSG ("SSL.sslfile->write: Propagating old callback error: %s\n",
			strerror (local_errno));
	RETURN (-1);
      }, 0);

    if (nonblocking_mode && SSL_HANDSHAKING) {
      SSL3_DEBUG_MSG ("SSL.sslfile->write: "
		      "Still in handshake - cannot accept application data\n");
      RETURN (0);
    }

    // Take care of any old data first.
    if (!direct_write()) RETURN (-1);

    int written = 0;

    if (arrayp (data)) {
      int idx = 0, pos = 0;

      while (idx < sizeof (data) && !sizeof (write_buffer) &&
	     // Always stop after writing DATA_CHUNK_SIZE in
	     // nonblocking mode, so that we don't loop here
	     // arbitrarily long if the write is very large and the
	     // bottleneck is in the encryption.
	     (!nonblocking_mode || written < Stdio.DATA_CHUNK_SIZE)) {
	int size = sizeof (data[idx]) - pos;
	if (size > SSL.Constants.PACKET_MAX_SIZE) {
	  // send_streaming_data will pick the first PACKET_MAX_SIZE
	  // bytes of the string, so do that right away in the same
	  // range operation.
	  int n = conn->send_streaming_data (
	    data[idx][pos..pos + SSL.Constants.PACKET_MAX_SIZE - 1]);
	  SSL3_DEBUG_MSG ("SSL.sslfile->write: Queued data[%d][%d..%d]\n",
			  idx, pos, pos + n - 1);
	  written += n;
	  pos += n;
	}

	else {
	  // Try to fill a packet.
	  int end;
	  for (end = idx + 1; end < sizeof (data); end++) {
	    int newsize = size + sizeof (data[end]);
	    if (newsize > SSL.Constants.PACKET_MAX_SIZE) break;
	    size = newsize;
	  }

	  if (conn->send_streaming_data (
		`+ (data[idx][pos..], @data[idx+1..end-1])) < size)
	    error ("Unexpected PACKET_MAX_SIZE discrepancy wrt send_streaming_data.\n");

	  SSL3_DEBUG_MSG ("SSL.sslfile->write: "
			  "Queued data[%d][%d..%d] + data[%d..%d]\n",
			  idx, pos, sizeof (data[idx]) - 1, idx + 1, end - 1);
	  written += size;
	  idx = end;
	  pos = 0;
	}

	if (!direct_write()) RETURN (written);
      }
    }

    else			// data is a string.
      while (written < sizeof (data) && !sizeof (write_buffer) &&
	     // Limit the amount written in a single call, for the
	     // same reason as above.
	     (!nonblocking_mode || written < Stdio.DATA_CHUNK_SIZE)) {
	int n = conn->send_streaming_data (
	  data[written..written + SSL.Constants.PACKET_MAX_SIZE - 1]);
	SSL3_DEBUG_MSG ("SSL.sslfile->write: Queued data[%d..%d]\n",
			written, written + n - 1);
	written += n;
	if (!direct_write()) RETURN (written);
      }

    SSL3_DEBUG_MSG ("SSL.sslfile->write: Write %t done, accepted %d bytes\n",
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
  SSL3_DEBUG_MSG ("SSL.sslfile->renegotiate()\n");

  ENTER (0, 0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    FIX_ERRNOS ({
	SSL3_DEBUG_MSG ("SSL.sslfile->renegotiate: "
			"Propagating old callback error: %s\n",
			strerror (local_errno));
	RETURN (0);
      }, 0);

    if (!stream) {
      SSL3_DEBUG_MSG ("SSL.sslfile->renegotiate: "
		      "Connection closed - simulating System.EPIPE\n");
      cleanup_on_error();
      local_errno = System.EPIPE;
      RETURN (0);
    }

    // FIXME: Change this state with a packet instead so that things
    // currently in the queue aren't affect by it.
    conn->expect_change_cipher = 0;
    conn->certificate_state = 0;
    conn->handshake_finished = 0;
    update_internal_state();

    conn->send_packet(conn->hello_request());

    RETURN (direct_write());
  } LEAVE;
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
  SSL3_DEBUG_MSG ("SSL.sslfile->set_callbacks (%O, %O, %O, %O, %O, %O)\n%s",
		  read, write, close, read_oob, write_oob, accept,
		  "" || describe_backtrace (backtrace()));

  ENTER(0, 0) {

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

    if (stream) update_internal_state();
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
  SSL3_DEBUG_MSG ("SSL.sslfile->set_nonblocking (%O, %O, %O, %O, %O, %O)\n%s",
		  read, write, close, read_oob, write_oob, accept,
		  "" || describe_backtrace (backtrace()));

  ENTER (0, 0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    nonblocking_mode = 1;

    accept_callback = accept;
    read_callback = read;
    write_callback = write;
    close_callback = close;

    if (stream) {
      stream->set_nonblocking_keep_callbacks();
      // Has to restore here since a backend waiting in another thread
      // might be woken immediately when callbacks are registered.
      RESTORE;
      update_internal_state();
      return;
    }
  } LEAVE;
}

void set_nonblocking_keep_callbacks()
//! Set nonblocking mode like @[set_nonblocking], but don't alter any
//! callbacks.
//!
//! @seealso
//!   @[set_nonblocking], @[set_blocking], @[set_blocking_keep_callbacks]
{
  SSL3_DEBUG_MSG ("SSL.sslfile->set_nonblocking_keep_callbacks()\n");

  ENTER (0, 0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    nonblocking_mode = 1;

    if (stream) {
      stream->set_nonblocking_keep_callbacks();
      // Has to restore here since a backend waiting in another thread
      // might be woken immediately when callbacks are registered.
      RESTORE;
      update_internal_state();
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

  SSL3_DEBUG_MSG ("SSL.sslfile->set_blocking()\n%s",
		  "" || describe_backtrace (backtrace()));

  ENTER (0, 0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    nonblocking_mode = 0;
    accept_callback = read_callback = write_callback = close_callback = 0;

    if (stream) {
      update_internal_state();
      stream->set_blocking();
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
  SSL3_DEBUG_MSG ("SSL.sslfile->set_blocking_keep_callbacks()\n");

  ENTER (0, 0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    nonblocking_mode = 0;

    if (stream) {
      update_internal_state();
      stream->set_blocking();
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
  CHECK (0, 0);
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
  SSL3_DEBUG_MSG ("SSL.sslfile->set_alert_callback (%O)\n", alert);
  CHECK (0, 0);
#ifdef DEBUG
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
//! The callback function will be called with the sslfile object
//! and the additional id arguments (set with @[set_id]).
//!
//! @note
//! Like the read, write and close callbacks, installing this callback
//! implies callback mode, even after the handshake is done.
//!
//! @seealso
//!   @[set_nonblocking], @[set_callbacks],
//!   @[query_accept_callback], @[query_callbacks]
{
  SSL3_DEBUG_MSG ("SSL.sslfile->set_accept_callback (%O)\n", accept);
  ENTER (0, 0) {
#ifdef DEBUG
    if (close_state == STREAM_UNINITIALIZED)
      error ("Doesn't have any connection.\n");
#endif
    accept_callback = accept;
    if (stream) update_internal_state();
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
  SSL3_DEBUG_MSG ("SSL.sslfile->set_read_callback (%O)\n", read);
  ENTER (0, 0) {
#ifdef DEBUG
    if (close_state == STREAM_UNINITIALIZED)
      error ("Doesn't have any connection.\n");
#endif
    read_callback = read;
    if (stream) update_internal_state();
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
  SSL3_DEBUG_MSG ("SSL.sslfile->set_write_callback (%O)\n", write);
  ENTER (0, 0) {
#ifdef DEBUG
    if (close_state == STREAM_UNINITIALIZED)
      error ("Doesn't have any connection.\n");
#endif
    write_callback = write;
    if (stream) update_internal_state();
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
  SSL3_DEBUG_MSG ("SSL.sslfile->set_close_callback (%O)\n", close);
  ENTER (0, 0) {
#ifdef DEBUG
    if (close_state == STREAM_UNINITIALIZED)
      error ("Doesn't have any connection.\n");
#endif
    close_callback = close;
    if (stream) update_internal_state();
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
  SSL3_DEBUG_MSG ("SSL.sslfile->set_id (%O)\n", id);
  CHECK (0, 0);
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
  ENTER (0, 0) {
    if (close_state > STREAM_OPEN) error ("Not open.\n");

    if (stream) {
      if (stream->query_backend() != local_backend)
	stream->set_backend (backend);

      if (got_extra_read_call_out > 0) {
	real_backend->remove_call_out (ssl_read_callback);
	backend->call_out (ssl_read_callback, 0, 1, 0);
      }
    }

    real_backend = backend;
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
  SSL3_DEBUG_MSG ("SSL.sslfile->is_open()\n");
  ENTER (0, 0) {
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
      if ((close_state == CLEAN_CLOSE ?
	   conn->closing != 3 : !conn->closing))
	RUN_MAYBE_BLOCKING (
	  action && (close_state == CLEAN_CLOSE ?
		     conn->closing != 3 : !conn->closing),
	  1, 1,
	  RETURN (!epipe_errnos[local_errno]));
      RETURN (conn && (close_state == CLEAN_CLOSE ?
		       conn->closing != 3 && 2 : !conn->closing));
    }
  } LEAVE;
  return 0;
}

Stdio.File query_stream()
//! Return the underlying stream.
//!
//! @note
//! Avoid any temptation to do
//! @expr{destruct(sslfile_obj->query_stream())@}. That almost
//! certainly creates more problems than it solves.
//!
//! You probably want to use @[shutdown].
//!
//! @seealso
//!   @[shutdown]
{
  SSL3_DEBUG_MSG ("SSL.sslfile->query_stream(): Called from %s:%d\n",
		  backtrace()[-2][0], backtrace()[-2][1]);
  return stream;
}

SSL.connection query_connection()
//! Return the SSL connection object.
//!
//! This returns the low-level @[SSL.connection] object.
{
  SSL3_DEBUG_MSG ("SSL.sslfile->query_connection(): Called from %s:%d\n",
		  backtrace()[-2][0], backtrace()[-2][1]);
  return conn;
}

SSL.context query_context()
//! Return the SSL context object.
{
  return conn && conn->context;
}

string _sprintf(int t) {
  return t=='O' && sprintf("SSL.sslfile(%O)", stream && stream->_fd);
}


protected void update_internal_state (void|int assume_real_backend)
// Update the internal callbacks according to the current state. Does
// nothing if the local backend is active, unless assume_real_backend
// is set, in which case we're installing callbacks for the real
// backend anyway (necessary to avoid races when we're about to switch
// from the local to the real backend).
{
  // When the local backend is used, callbacks are set explicitly
  // before it's started.
  if (assume_real_backend || stream->query_backend() != local_backend) {
    mixed install_read_cbs, install_write_cb;

    if (nonblocking_mode &&
	(SSL_HANDSHAKING || close_state >= NORMAL_CLOSE)) {
      // Normally we never install our own callbacks if we aren't in
      // callback mode, but handling a nonblocking close is an
      // exception.
      install_read_cbs = SSL_INTERNAL_READING;
      install_write_cb = SSL_INTERNAL_WRITING;

      SSL3_DEBUG_MORE_MSG ("update_internal_state: "
			   "%s [r:%O w:%O rcb:%O]\n",
			   SSL_HANDSHAKING ? "Handshaking" : "After close",
			   !!install_read_cbs, !!install_write_cb,
			   got_extra_read_call_out);
    }

    // CALLBACK_MODE but slightly optimized below.
    else if (read_callback || write_callback || close_callback || accept_callback) {
      install_read_cbs = (read_callback || close_callback || accept_callback ||
			  SSL_INTERNAL_READING);
      install_write_cb = (write_callback || SSL_INTERNAL_WRITING);

      SSL3_DEBUG_MORE_MSG ("update_internal_state: "
			   "Callback mode [r:%O w:%O rcb:%O]\n",
			   !!install_read_cbs, !!install_write_cb,
			   got_extra_read_call_out);
    }

    else {
      // Not in callback mode. Can't install callbacks even though we'd
      // "need" to - have to cope with the local backend in each
      // operation instead.
      SSL3_DEBUG_MORE_MSG ("update_internal_state: "
			   "Not in callback mode [rcb:%O]\n",
			   got_extra_read_call_out);
    }

    if (!got_extra_read_call_out && sizeof (read_buffer) && read_callback)
      // Got buffered read data and there's someone ready to receive it,
      // so schedule a call to ssl_read_callback to handle it.
      got_extra_read_call_out = -1;

    // If got_extra_read_call_out is set here then we wait with all
    // other callbacks so that the extra ssl_read_callback call is
    // carried out before anything else.

    if (install_read_cbs) {
      if (got_extra_read_call_out < 0) {
	real_backend->call_out (ssl_read_callback, 0, 1, 0);
	got_extra_read_call_out = 1;
      }
      else {
	stream->set_read_callback (ssl_read_callback);
	stream->set_close_callback (ssl_close_callback);
      }
    }
    else {
      stream->set_read_callback (0);
      // Installing a close callback without a read callback
      // currently doesn't work well in Stdio.File.
      stream->set_close_callback (0);
      if (got_extra_read_call_out > 0) {
	real_backend->remove_call_out (ssl_read_callback);
	got_extra_read_call_out = -1;
      }
    }

    stream->set_write_callback (install_write_cb && !got_extra_read_call_out &&
				ssl_write_callback);

#ifdef DEBUG
    if (!assume_real_backend && op_thread)
      // Check that we haven't installed callbacks that might start
      // executing in parallell in another thread. That's legitimate
      // in some cases (e.g. set_nonblocking) but in those cases we're
      // called after RESTORE or LEAVE, which has reset op_thread, so
      // we skip this check if op_thread is zero.
      CHECK_CB_MODE (THIS_THREAD());
#endif
  }

  else
    SSL3_DEBUG_MORE_MSG ("update_internal_state: "
			 "In local backend - nothing done [rcb:%O]\n",
			 got_extra_read_call_out);
}

protected int queue_write()
// Return 0 if the connection is still alive, 1 if it was closed
// politely, and -1 if it died unexpectedly (specifically, our side
// has sent a fatal alert packet (not close notify) for some reason
// and therefore nullified the connection).
{
  int|string res = conn->to_write();

#ifdef SSL3_DEBUG_TRANSPORT
  werror ("queue_write: To write: %O\n", res);
#endif

  if (stringp (res)) {
    if (res == "")
      SSL3_DEBUG_MSG ("queue_write: Got nothing to write (%d strings buffered)\n",
		      sizeof (write_buffer));

    else {
      int was_empty = !sizeof (write_buffer);
      write_buffer += ({res});
      SSL3_DEBUG_MSG ("queue_write: Got %d bytes to write (%d strings buffered)\n",
		      sizeof (res), sizeof (write_buffer));
      if (was_empty && stream)
	update_internal_state();
    }

    return 0;
  }

  SSL3_DEBUG_MSG ("queue_write: Connection closed %s\n",
		  res == 1 ? "normally" : "abruptly");

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
			  nonblocking_mode, SSL_INTERNAL_READING,
			  SSL3_DEBUG_MORE_MSG ("direct_write: Got error\n");
			  return 0;);
  }

  SSL3_DEBUG_MORE_MSG ("direct_write: Ok\n");
  return 1;
}

protected int ssl_read_callback (int called_from_real_backend, string input)
{
  SSL3_DEBUG_MSG ("ssl_read_callback (%O, %s): "
		  "nonblocking mode=%d, callback mode=%d%s%s\n",
		  called_from_real_backend,
		  input ? "string[" + sizeof (input) + "]" : "0 (queued extra call)",
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn && SSL_HANDSHAKING ? ", handshaking" : "",
		  conn && SSL_CLOSING_OR_CLOSED ?
		  ", closing (" + conn->closing + ", " + close_state + ", " +
		  close_packet_send_state + ")" : "");

  ENTER (1, called_from_real_backend) {
    int call_accept_cb;

    if (input) {
      int handshake_already_finished = conn->handshake_finished;
      string|int data =
	close_state == ABRUPT_CLOSE ? -1 : conn->got_data (input);

#ifdef DEBUG
      if (got_extra_read_call_out)
	error ("Got to real read callback with queued extra read call out.\n");
#endif

#ifdef SSL3_DEBUG_TRANSPORT
      werror ("ssl_read_callback: Got data: %O\n", data);
#endif

      if (alert_cb_called)
	SSL3_DEBUG_MSG ("ssl_read_callback: After alert cb call\n");

      else {
	int write_res;
	if (stringp(data) || (data > 0) ||
	    ((data < 0) && !conn->handshake_finished)) {
#ifdef DEBUG
	  if (!stream)
	    error ("Got zapped stream in callback.\n");
#endif
	  // got_data might have put more packets in the write queue if
	  // we're handshaking.
	  write_res = queue_write();
	}

	cb_errno = 0;

	if (stringp (data)) {
	  if (!handshake_already_finished && conn->handshake_finished) {
	    SSL3_DEBUG_MSG ("ssl_read_callback: Handshake finished\n");
	    update_internal_state();
	    if (called_from_real_backend && accept_callback) {
#ifdef DEBUG
	      if (close_state >= NORMAL_CLOSE)
		error ("Didn't expect the connection to be "
		       "explicitly closed already.\n");
#endif
	      call_accept_cb = 1;
	    }
	  }

	  SSL3_DEBUG_MSG ("ssl_read_callback: "
			  "Got %d bytes of application data\n", sizeof (data));
	  read_buffer->add (data);
	}

	else if (data < 0 || write_res < 0) {
	  SSL3_DEBUG_MSG ("ssl_read_callback: "
			  "Got abrupt remote close - simulating System.EPIPE\n");
	  cleanup_on_error();
	  cb_errno = System.EPIPE;
	}

	// Don't use data > 0 here since we might have processed some
	// application data and a close in the same got_data call.
	if (conn->closing & 2) {
	  if (close_packet_send_state == CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR) {
	    SSL3_DEBUG_MSG ("ssl_read_callback: Got close packet - "
			    "ignoring failure to send one\n");
	    close_packet_send_state = CLOSE_PACKET_QUEUED_OR_DONE;
	  }
	  else
	    SSL3_DEBUG_MSG ("ssl_read_callback: Got close packet\n");
	}
      }
    }

    else {
      // This is another call that has been queued below through a
      // call out. That is necessary whenever we need to call several
      // user callbacks from the same invocation: We can't do anything
      // after calling a user callback, since the user is free to e.g.
      // remove the callbacks and "hand over" us to another thread and
      // do more I/O there while this one returns. We solve this
      // problem by queuing a call out to ourselves (with zero as
      // input) to continue.
      got_extra_read_call_out = 0;
      update_internal_state();
    }

    // Figure out what we need to do. call_accept_cb is already set
    // from above.
    int(0..1) call_read_cb;
    int(0..1) do_close_stuff;
    if (alert_cb_called) {
      if (!conn) {
	SSL3_DEBUG_MSG ("ssl_read_callback: Shut down from alert callback\n");
	RESTORE;
	return 0;
      }
      // Make sure the alert is queued for writing.
      queue_write();
    }
    else {
      call_read_cb =
	called_from_real_backend && read_callback && sizeof (read_buffer) &&
	// Shouldn't get here when close_state == ABRUPT_CLOSE.
	close_state < NORMAL_CLOSE;
      do_close_stuff =
        !!((conn->closing & 2) || cb_errno) && !sizeof(read_buffer);
    }

    if (alert_cb_called || call_accept_cb + call_read_cb + do_close_stuff > 1) {
      // Need to do a call out to ourselves; see comment above.
#ifdef DEBUG
      if (!alert_cb_called && !called_from_real_backend)
	error ("Internal confusion.\n");
      if (called_from_real_backend && got_extra_read_call_out < 0)
	error ("Ended up in ssl_read_callback from real backend "
	       "when no callbacks are supposed to be installed.\n");
#endif
      if (!got_extra_read_call_out) {
	got_extra_read_call_out = -1;
	SSL3_DEBUG_MSG ("ssl_read_callback: Too much to do (%O, %O, %O, %O) - "
			"scheduled another call\n", alert_cb_called, call_accept_cb,
			call_read_cb, do_close_stuff);
	update_internal_state();
      }
      else
	SSL3_DEBUG_MSG ("ssl_read_callback: Too much to do (%O, %O, %O, %O) - "
			"another call already queued\n",
			alert_cb_called, call_accept_cb, call_read_cb, do_close_stuff);
      alert_cb_called = 0;
    }

    else
      if (got_extra_read_call_out) {
	// Don't know if this actually can happen, but it's symmetric.
	if (got_extra_read_call_out > 0)
	  real_backend->remove_call_out (ssl_read_callback);
	got_extra_read_call_out = 0;
	update_internal_state();
      }

    // Now actually do (a bit of) what should be done.

    if (call_accept_cb) {
      SSL3_DEBUG_MSG ("ssl_read_callback: Calling accept callback %O\n",
		      accept_callback);
      RESTORE;
      return accept_callback (this, callback_id);
    }

    else if (call_read_cb) {
      string received = read_buffer->get();
      SSL3_DEBUG_MSG ("call_read_callback: Calling read callback %O "
		      "with %d bytes\n", read_callback, sizeof (received));
      // Never called if there's an error - no need to propagate cb_errno.
      RESTORE;
      return read_callback (callback_id, received);
    }

    else if (do_close_stuff) {
#ifdef DEBUG
      if (got_extra_read_call_out)
	error ("Shouldn't have more to do after close stuff.\n");
#endif

      if (conn->closing & 2 &&
	  close_packet_send_state == CLOSE_PACKET_NOT_SCHEDULED) {
	close_packet_send_state = CLOSE_PACKET_SCHEDULED;
	// Deinstall read side cbs to avoid reading more and install
	// the write cb to send the close packet. Can't use
	// update_internal_state here since we should force a
	// deinstall of the read side cbs even when we're still in
	// callback mode, to correctly imitate the behavior in
	// Stdio.File (it deinstalls read side cbs whenever the close
	// cb is called, but it's possible to reinstall the close cb
	// later and get another call to it).
	if (stream->query_backend() != local_backend) {
	  stream->set_read_callback (0);
	  stream->set_close_callback (0);
	  stream->set_write_callback (ssl_write_callback);
	  SSL3_DEBUG_MORE_MSG ("ssl_read_callback: Setting cbs for close [r:0 w:1]\n");
	}
      }

      if (called_from_real_backend && close_callback) {
	// errno() should return the error in the close callback - need to
	// propagate it here.
	FIX_ERRNOS (
	  SSL3_DEBUG_MSG ("ssl_read_callback: Calling close callback %O (error %s)\n",
			  close_callback, strerror (local_errno)),
	  SSL3_DEBUG_MSG ("ssl_read_callback: Calling close callback %O (read eof)\n",
			  close_callback)
	);
	RESTORE;
	return close_callback (callback_id);
      }

      if (close_state >= NORMAL_CLOSE) {
	SSL3_DEBUG_MSG ("ssl_read_callback: "
			"In or after local close - shutting down\n");
	shutdown();
      }

      if (cb_errno) {
	SSL3_DEBUG_MSG ("ssl_read_callback: Returning with error\n");
	// Make sure the local backend exits after this, so that the
	// error isn't clobbered by later I/O.
	RESTORE;
	return -1;
      }
    }

  } LEAVE;
  return 0;
}

protected int ssl_write_callback (int called_from_real_backend)
{
  SSL3_DEBUG_MSG ("ssl_write_callback (%O): "
		  "nonblocking mode=%d, callback mode=%d%s%s\n",
		  called_from_real_backend,
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn && SSL_HANDSHAKING ? ", handshaking" : "",
		  conn && SSL_CLOSING_OR_CLOSED ?
		  ", closing (" + conn->closing + ", " + close_state + ", " +
		  close_packet_send_state + ")" : "");

  int ret = 0;

  ENTER (1, called_from_real_backend) {
#ifdef DEBUG
    if (!stream)
      error ("Got zapped stream in callback.\n");
    if (got_extra_read_call_out)
      error ("Got to write callback with queued extra read call out.\n");
#endif

  write_to_stream:
    do {
      if (sizeof (write_buffer)) {
	string output = write_buffer[0];
	int written;
#ifdef SIMULATE_CLOSE_PACKET_WRITE_FAILURE
	if (close_packet_send_state == CLOSE_PACKET_QUEUED_OR_DONE)
	  written = -1;
	else
#endif
	  written = stream->write (output);

	if (written < 0) {
#ifdef SIMULATE_CLOSE_PACKET_WRITE_FAILURE
	  if (close_packet_send_state == CLOSE_PACKET_QUEUED_OR_DONE)
	    cb_errno = System.EPIPE;
	  else
#endif
	    cb_errno = stream->errno();
	  cleanup_on_error();
	  SSL3_DEBUG_MSG ("ssl_write_callback: Write failed: %s\n",
			  strerror (cb_errno));

	  // Make sure the local backend exits after this, so that the
	  // error isn't clobbered by later I/O.
	  ret = -1;

	  if (close_packet_send_state == CLOSE_PACKET_QUEUED_OR_DONE &&
	      epipe_errnos[cb_errno]) {
	    // See if it's an error from writing a close packet that
	    // should be ignored.
	    write_buffer = ({}); // No use trying to write the close again.

	    if (close_state == CLEAN_CLOSE) {
	      // Never accept a failure if a clean close is requested.
	      close_packet_send_state = CLOSE_PACKET_WRITE_ERROR;
	      update_internal_state();
	    }

	    else {
	      cb_errno = 0;

	      if (conn->closing & 2)
		SSL3_DEBUG_MSG ("ssl_write_callback: Stream closed properly "
				"remotely - ignoring failure to send close packet\n");

	      else {
		SSL3_DEBUG_MSG ("ssl_write_callback: Stream closed remotely - "
				"checking input buffer for proper remote close\n");
		// This causes the read/close callbacks to be
		// installed to handle the close packet that might be
		// sitting in the input buffer.
		close_packet_send_state = CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR;
		update_internal_state();
		if (called_from_real_backend) {
		  // Shouldn't wait for a close packet that might
		  // arrive later on, so we start a nonblocking local
		  // backend to check for it. If we're already in a
		  // local backend, this is handled by special cases
		  // in RUN_MAYBE_BLOCKING.
		  RUN_MAYBE_BLOCKING (
		    close_packet_send_state == CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR,
		    1, 1, {});
		}
		else {
		  // Can't start a nested local backend - skip out to
		  // the one we're called from.
		  RESTORE;
		  return ret;
		}
	      }
	    }
	  }

	  // Still try to call the write (or close) callback to
	  // propagate the error to it.
	  break write_to_stream;
	}

	if (written == sizeof (output))
	  write_buffer = write_buffer[1..];
	else
	  write_buffer[0] = output[written..];

	SSL3_DEBUG_MSG ("ssl_write_callback: Wrote %d bytes (%d strings left)\n",
			written, sizeof (write_buffer));
	if (sizeof (write_buffer)) {
	  RESTORE;
	  return ret;
	}
	update_internal_state();
      }

      else
	// Ensure that the close is sent separately in case we should
	// ignore the error from it.
	if (close_packet_send_state == CLOSE_PACKET_SCHEDULED) {
	  if (linger_time) {
	    SSL3_DEBUG_MSG ("ssl_write_callback: Queuing close packet\n");
	    conn->send_close();
	  } else {
	    SSL3_DEBUG_MSG ("ssl_write_callback: Skipping close packet\n");
	  }
	  close_packet_send_state = CLOSE_PACKET_QUEUED_OR_DONE;
	}

      if (int err = queue_write()) {
	if (err > 0) {
#ifdef DEBUG
	  if (!conn->closing || close_packet_send_state < CLOSE_PACKET_QUEUED_OR_DONE)
	    error ("Expected a close to be sent or received\n");
#endif

	  if (sizeof (write_buffer))
	    SSL3_DEBUG_MSG ("ssl_write_callback: "
			    "Close packet queued but not yet sent\n");
	  else {
#ifdef DEBUG
	    if (!(conn->closing & 1))
	      error ("Expected a close packet to be queued or sent.\n");
#endif
	    if (conn->closing == 3 || close_state != CLEAN_CLOSE) {
	      SSL3_DEBUG_MSG ("ssl_write_callback: %s\n",
			      conn->closing == 3 ?
			      "Close packets exchanged" : "Close packet sent");
	      break write_to_stream;
	    }
	    else {
	      SSL3_DEBUG_MSG ("ssl_write_callback: "
			      "Close packet sent - expecting response\n");
	      // Not SSL_INTERNAL_WRITING anymore.
	      update_internal_state();
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
	  cb_errno = System.EPIPE;
	  ret = -1;
	  break write_to_stream;
	}
      }
    } while (sizeof (write_buffer));

    if (called_from_real_backend) {
      if ((close_packet_send_state >= CLOSE_PACKET_SCHEDULED) &&
          !sizeof(read_buffer)) {
	if (close_callback && cb_errno && close_state < NORMAL_CLOSE) {
	  // Better signal errors writing the close packet to the
	  // close callback.
	  FIX_ERRNOS (
	    SSL3_DEBUG_MSG ("ssl_write_callback: Calling close callback %O "
			    "(error %s)\n", close_callback, strerror (local_errno)),
	    0
	  );
	  RESTORE;
	  return close_callback (callback_id);
	}
      }

      if (write_callback && !SSL_HANDSHAKING
	  && (close_state < NORMAL_CLOSE || cb_errno)) {
	// errno() should return the error in the write callback - need
	// to propagate it here.
	FIX_ERRNOS (
	  SSL3_DEBUG_MSG ("ssl_write_callback: Calling write callback %O "
			  "(error %s)\n", write_callback, strerror (local_errno)),
	  SSL3_DEBUG_MSG ("ssl_write_callback: Calling write callback %O\n",
			  write_callback)
	);
	RESTORE;
	return write_callback (callback_id);
      }
    }

    if (close_state >= NORMAL_CLOSE &&
	(close_packet_send_state >= CLOSE_PACKET_QUEUED_OR_DONE || cb_errno)) {
#ifdef DEBUG
      if (close_packet_send_state == CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR)
	error ("Unexpected close_packet_send_state\n");
#endif
      SSL3_DEBUG_MSG ("ssl_write_callback: "
		      "In or after local close - shutting down\n");
      shutdown();
    }
  } LEAVE;
  return ret;
}

protected int ssl_close_callback (int called_from_real_backend)
{
  SSL3_DEBUG_MSG ("ssl_close_callback (%O): "
		  "nonblocking mode=%d, callback mode=%d%s%s\n",
		  called_from_real_backend,
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn && SSL_HANDSHAKING ? ", handshaking" : "",
		  conn && SSL_CLOSING_OR_CLOSED ?
		  ", closing (" + conn->closing + ", " + close_state + ", " +
		  close_packet_send_state + ")" : "");

  ENTER (1, called_from_real_backend) {
#ifdef DEBUG
    if (!stream)
      error ("Got zapped stream in callback.\n");
    if (got_extra_read_call_out)
      error ("Got to close callback with queued extra read call out.\n");
#endif

    // If we've arrived here due to an error, let it override any
    // older errno from an earlier callback.
    if (int new_errno = stream->errno()) {
      SSL3_DEBUG_MSG ("ssl_close_callback: Got error %s\n", strerror (new_errno));
      cleanup_on_error();
      cb_errno = new_errno;
    }
#ifdef SSL3_DEBUG
    else if (cb_errno)
      SSL3_DEBUG_MSG ("ssl_close_callback: Propagating errno from another callback\n");
#endif

    if (!cb_errno) {
      if (conn->closing & 2)
	SSL3_DEBUG_MSG ("ssl_close_callback: After clean close\n");

      else {
	// The remote end has closed the connection without sending a
	// close packet. Treat that as an error so that the caller can
	// detect truncation attacks.
	if (close_packet_send_state == CLOSE_PACKET_MAYBE_IGNORED_WRITE_ERROR) {
	  SSL3_DEBUG_MSG ("ssl_close_callback: Did not get a remote close - "
			  "signalling delayed error from writing close message\n");
	  close_packet_send_state = CLOSE_PACKET_WRITE_ERROR;
	}
	else
	  SSL3_DEBUG_MSG ("ssl_close_callback: Abrupt close - "
			  "simulating System.EPIPE\n");
	cleanup_on_error();
	cb_errno = System.EPIPE;
	close_state = ABRUPT_CLOSE;
      }
    }

    if (!sizeof(read_buffer)) {
      if (called_from_real_backend && close_callback) {
        // errno() should return the error in the close callback - need to
        // propagate it here.
        FIX_ERRNOS (
          SSL3_DEBUG_MSG ("ssl_close_callback: Calling close callback %O (error %s)\n",
                          close_callback, strerror (local_errno)),
          SSL3_DEBUG_MSG ("ssl_close_callback: Calling close callback %O (read eof)\n",
                          close_callback)
        );
        RESTORE;
        // Note that the callback should call close() (or free things
        // so that we get destructed) - there's no need for us to
        // schedule a shutdown after it.
        return close_callback (callback_id);
      }

      if (close_state >= NORMAL_CLOSE) {
        SSL3_DEBUG_MSG ("ssl_close_callback: "
                        "In or after local close - shutting down\n");
        shutdown();
      }
    }
  } LEAVE;

  // Make sure the local backend exits after this, so that the error
  // isn't clobbered by later I/O.
  return -1;
}

//! The next protocol chosen by the client during next protocol
//! negotiation.
string `->next_protocol() {
    return conn->next_protocol;
}

#else // constant(SSL.Cipher.CipherAlgorithm)
constant this_program_does_not_exist = 1;
#endif
