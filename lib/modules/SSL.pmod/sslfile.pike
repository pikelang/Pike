#pike __REAL_VERSION__

/* $Id: sslfile.pike,v 1.88 2005/01/26 21:35:08 mast Exp $
 */

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
//!   Connection init (@[create]) and close (@[close]) can do both
//!   reading and writing.
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
static string stream_descr;
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

static Stdio.File stream;
// The stream is closed as soon as possible to avoid garbage. That
// means as soon as 1) close messages have been exchanged according to
// the close mode (see clean_close for close()), or 2) a remote abrupt
// close or stream close has been discovered (no use sending a close
// message then).

static SSL.connection conn;
// Always set when stream is. Destructed with destroy() at shutdown
// since it contains cyclic references. Noone else gets to it, though.

static array(string) write_buffer; // Encrypted data to be written.
static String.Buffer read_buffer; // Decrypted data that has been read.

static mixed callback_id;
static function(void|object,void|mixed:int) accept_callback;
static function(void|mixed,void|string:int) read_callback;
static function(void|mixed:int) write_callback;
static function(void|mixed:int) close_callback;

static Pike.Backend real_backend;
// The real backend for the stream.

static Pike.Backend local_backend;
// Internally all I/O is done using callbacks. When the real backend
// can't be used, either because we aren't in callback mode (i.e. the
// user hasn't registered any callbacks), or because we're to do a
// blocking operation, this local one takes its place. It's
// created on demand.

static int nonblocking_mode;

static enum CloseState {
  CLOSE_CB_CALLED = -1,
  STREAM_OPEN = 0,
  STREAM_UNINITIALIZED = 1,
  NORMAL_CLOSE = 2,		// The caller has requested a normal close.
  CLEAN_CLOSE = 3,		// The caller has requested a clean close.
}
static CloseState close_state = STREAM_UNINITIALIZED;

static int conn_closing;
// This is conn->closing after shutdown when conn has been destructed.

static int local_errno;
// If nonzero, override the errno on the stream with this.

static int cb_errno;
// Stores the errno from failed I/O in a callback so that the next
// visible I/O operations can report it properly.

static int got_extra_read_call_out;
// 1 when we have a call out to ssl_read_callback. We get this when we
// need to call read_callback or close_callback but can't do that
// right away from ssl_read_callback. See comments in that function
// for more details. -1 if we've switched to non-callback mode and
// therefore has removed the call out temporarily but need to restore
// it when switching back. 0 otherwise.

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

#define CALLBACK_MODE							\
  (read_callback || write_callback || close_callback || accept_callback)

#define SSL_HANDSHAKING (!conn->handshake_finished)
#define SSL_READING_CLEAN_CLOSE (close_state == CLEAN_CLOSE && conn->closing == 1)
#define SSL_INTERNAL_TALK (SSL_HANDSHAKING || SSL_READING_CLEAN_CLOSE)

#ifdef DEBUG

#if constant (Thread.thread_create)

#define THREAD_T Thread.Thread
#define THIS_THREAD() this_thread()

static void thread_error (string msg, THREAD_T other_thread)
{
  error ("%s"
	 "User callbacks: a=%O r=%O w=%O c=%O\n"
	 "Internal callbacks: r=%O w=%O c=%O\n"
	 "Backend: %O  This thread: %O  Other thread: %O\n"
	 "Other thread backtrace:\n%s----------\n",
	 msg,
	 accept_callback, read_callback, write_callback, close_callback,
	 stream && stream->query_read_callback(),
	 stream && stream->query_write_callback(),
	 stream && stream->query_close_callback(),
	 stream && stream->query_backend(),
	 this_thread(), other_thread,
	 describe_backtrace (other_thread->backtrace()));
}

#else  // !constant (Thread.thread_create)

#define THREAD_T int
#define THIS_THREAD() 1

static void thread_error (string msg, THREAD_T other_thread)
{
  error ("%s"
	 "User callbacks: a=%O r=%O w=%O c=%O\n"
	 "Internal callbacks: r=%O w=%O c=%O\n"
	 "Backend: %O\n",
	 msg,
	 accept_callback, read_callback, write_callback, close_callback,
	 stream && stream->query_read_callback(),
	 stream && stream->query_write_callback(),
	 stream && stream->query_close_callback(),
	 stream && stream->query_backend());
}

#endif	// !constant (Thread.thread_create)

static THREAD_T op_thread;

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
      thread_error ("Already doing operation in another thread.\n",	\
		    OP_THREAD);						\
									\
    if (Pike.Backend backend = local_backend || real_backend)		\
      if (THREAD_T backend_thread = backend->executing_thread())	\
	if (backend_thread != CUR_THREAD &&				\
	    stream && (stream->query_read_callback() ||			\
		       stream->query_write_callback() ||		\
		       stream->query_close_callback()))			\
	  thread_error ("Already in callback mode "			\
			"in a backend in another thread.\n",		\
			backend_thread);				\
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

#define CHECK(IN_CALLBACK, CALLED_FROM_REAL_BACKEND) do {} while (0)
#define ENTER(IN_CALLBACK, CALLED_FROM_REAL_BACKEND) do
#define RESTORE do {} while (0)
#define RETURN(RET_VAL) return (RET_VAL)
#define LEAVE while (0)

#endif	// !DEBUG

// stream is assumed to be operational on entry but might be zero
// afterwards. cb_errno is assumed to be 0 on entry.
#define RUN_MAYBE_BLOCKING(COND, NONBLOCKING_MODE, ENABLE_READS, ERROR_CODE) do { \
  run_maybe_blocking:							\
    if (COND) {								\
      if (!local_backend) local_backend = Pike.Backend();		\
      stream->set_backend (local_backend);				\
      stream->set_id (0);						\
      SSL3_DEBUG_MSG ("Starting %s local backend\n",			\
		      NONBLOCKING_MODE ? "nonblocking" : "blocking");	\
									\
      while (1) {							\
	float|int(0..0) action;						\
									\
	if (got_extra_read_call_out && ENABLE_READS) {			\
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
	}								\
									\
	else {								\
	  stream->set_write_callback (sizeof (write_buffer) && ssl_write_callback); \
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
	  SSL3_DEBUG_MORE_MSG ("Running local backend [%O %O %O]\n",	\
			       stream->query_read_callback(),		\
			       stream->query_write_callback(),		\
			       stream->query_close_callback());		\
									\
	  action = local_backend (NONBLOCKING_MODE ? 0.0 : 0);		\
	}								\
									\
	FIX_ERRNOS ({							\
	    SSL3_DEBUG_MSG ("%s local backend ended with error\n",	\
			    NONBLOCKING_MODE ? "Nonblocking" : "Blocking"); \
	    if (stream) {						\
	      stream->set_backend (real_backend);			\
	      stream->set_id (1);					\
	      update_internal_state();					\
	    }								\
	    {ERROR_CODE;}						\
	    break run_maybe_blocking;					\
	  }, 0);							\
									\
	if (!stream) {							\
	  SSL3_DEBUG_MSG ("%s local backend ended after clean close\n",	\
			  NONBLOCKING_MODE ? "Nonblocking" : "Blocking"); \
	  break run_maybe_blocking;					\
	}								\
									\
	if (NONBLOCKING_MODE && !action) {				\
	  SSL3_DEBUG_MSG ("Nonblocking local backend ended - nothing to do\n"); \
	  break;							\
	}								\
									\
	if (!(COND)) {							\
	  SSL3_DEBUG_MSG ("%s local backend ended\n",			\
			  NONBLOCKING_MODE ? "Nonblocking" : "Blocking"); \
	  break;							\
	}								\
									\
	SSL3_DEBUG_MSG ("Reentering %s backend\n",			\
			NONBLOCKING_MODE ? "nonblocking" : "blocking");	\
      }									\
									\
      stream->set_backend (real_backend);				\
      stream->set_id (1);						\
      update_internal_state();						\
    }									\
  } while (0)

static void create (Stdio.File stream, SSL.context ctx,
		    int|void is_client, int|void is_blocking)
//! Create a connection over @[stream], which should be an open socket or
//! pipe. @[ctx] is the SSL context. If @[is_client] is set then a
//! client-side connection is started, server-side otherwise. If
//! @[is_blocking] is set then the stream is initially set in blocking
//! mode, nonblocking mode otherwise.
//!
//! The backend used by @[stream] is taken over and restored after the
//! connection is closed (see @[close]). The callbacks and id in
//! @[stream] are overwritten.
{
  SSL3_DEBUG_MSG ("SSL.sslfile->create (%O, %O, %O, %O)\n",
		  stream, ctx, is_client, is_blocking);

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
    callback_id = 0;
    real_backend = stream->query_backend();
    local_backend = 0;
    close_state = STREAM_OPEN;
    local_errno = cb_errno = 0;
    got_extra_read_call_out = 0;

    stream->set_read_callback (0);
    stream->set_write_callback (0);
    stream->set_close_callback (0);
    stream->set_id (1);

    if(!ctx->random)
      ctx->random = Crypto.Random.random_string;
    conn = SSL.connection (!is_client, ctx);

    if(is_blocking) {
      set_blocking();
      if (is_client) direct_write();
    }
    else
      set_nonblocking();
  } LEAVE;
}

//! returns peer certificate information, if any.
mapping get_peer_certificate_info()
{
  return conn->session->cert_data;
}

//! returns the peer certificate chain, if any.
array get_peer_certificates()
{
  return conn->session->peer_certificate_chain;
}

int close (void|string how, void|int clean_close)
//! Close the connection. Both the read and write ends are always
//! closed - the argument @[how] is only for @[Stdio.File]
//! compatibility and must be either @expr{"rw"@} or @expr{0@}.
//!
//! If @[clean_close] is set then close messages are exchanged to shut
//! down the SSL connection but not the underlying stream. It may then
//! continue to be used for other communication afterwards. The
//! default is to send a close message and then close the stream
//! without waiting for a response.
//!
//! Returns zero and sets the errno to @[System.EBADF] if the
//! connection already is closed. Other I/O errors are thrown.
//!
//! @note
//! In nonblocking mode the stream isn't closed right away and the
//! backend will be used for a while afterwards. If there's an I/O
//! problem it won't be signalled immediately.
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
{
  SSL3_DEBUG_MSG ("SSL.sslfile->close (%O, %O)\n", how, clean_close);

  if (how && how != "rw")
    error ("Can only close the connection in both directions simultaneously.\n");

  ENTER (0, 0) {
    if (close_state > 0) {
      SSL3_DEBUG_MSG ("SSL.sslfile->close: Already closed (%d)\n", close_state);
      local_errno = System.EBADF;
      RETURN (0);
    }
    close_state = clean_close ? CLEAN_CLOSE : NORMAL_CLOSE;

    FIX_ERRNOS ({
	// Get here if a close callback calls close after an error.
	SSL3_DEBUG_MSG ("SSL.sslfile->close: Shutdown after error\n");
	shutdown();
	RETURN (0);
      }, 0);

    if (!stream)
      if (conn_closing == 3) {
	SSL3_DEBUG_MSG ("SSL.sslfile->close: Already closed cleanly remotely\n");
	RETURN (1);
      }
      else {
	local_errno = System.EPIPE;
	// Errors are thrown from close().
	error ("Failed to close SSL connection: Got abrupt remote close.\n");
      }

    conn->send_close();
    update_internal_state();	// conn->closing changed.

    if (!direct_write())
      // Errors are thrown from close().
      error ("Failed to close SSL connection: %s\n", strerror (errno()));

    SSL3_DEBUG_MSG ("SSL.sslfile->close: Close %s\n", stream ? "underway" : "done");
    RETURN (1);
  } LEAVE;
}

Stdio.File shutdown()
//! Shut down the SSL connection without sending any more packets. The
//! underlying stream is returned if the connection isn't shut down
//! already and if a nonclean close hasn't been requested.
{
  ENTER (0, 0) {
    if (!stream) {
      SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): Already shut down\n");
      RETURN (0);
    }

    conn_closing = conn->closing;
    if (close_state == NORMAL_CLOSE)
      // If we didn't request a clean close then we pretend to have
      // received a close message. According to the standard it's ok
      // anyway as long as the transport isn't used for anything else.
      conn_closing |= 2;

    SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): %s close\n",
		    conn_closing == 3 && !sizeof (write_buffer) ?
		    "Clean" : "Abrupt");

    if ((conn_closing & 2) && sizeof (conn->left_over || "")) {
#ifdef DEBUG
      werror ("Warning: Got buffered data after close in %O: %O%s\n", this,
	      conn->left_over[..99], sizeof (conn->left_over) > 100 ? "..." : "");
#endif
      read_buffer = String.Buffer (sizeof (conn->left_over));
      read_buffer->add (conn->left_over);
      close_state = STREAM_OPEN;
    }

    if (conn->session && !sizeof(conn->session->identity))
      // conn->session doesn't exist before the handshake.
      conn->context->purge_session (conn->session);
    destruct (conn);		// Necessary to avoid garbage.

    write_buffer = ({});

    accept_callback = 0;
    read_callback = 0;
    write_callback = 0;
    close_callback = 0;

    if (got_extra_read_call_out > 0)
      real_backend->remove_call_out (ssl_read_callback);
    got_extra_read_call_out = 0;

    Stdio.File stream = global::stream;
    global::stream = 0;

    stream->set_read_callback (0);
    stream->set_write_callback (0);
    stream->set_close_callback (0);

    if (close_state == NORMAL_CLOSE) {
      SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): Normal close - closing stream\n");
      stream->close();
      RETURN (0);
    }
    else {
      // This also happens if we've been called due to a remote close.
      // Since all references to and in the stream object has been
      // removed it'll run out of refs and be closed shortly anyway.
      SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): Leaving stream\n");
      RETURN (stream);
    }
  } LEAVE;
}

static void destroy()
// Try to close down the connection properly in blocking mode since
// it's customary to close files just by dropping them.
{
  SSL3_DEBUG_MSG ("SSL.sslfile->destroy()\n");

  // Note that we don't know which thread this will be called in
  // (could be anyone if the gc got here), so there might be a race
  // problem with a backend in another thread. That would only
  // happen if somebody has destructed this object explicitly
  // though, and in that case he can have all that's coming.
  ENTER (0, 0) {
    if (stream && !close_state &&
	// Don't bother with closing nicely if there's an error from
	// an earlier operation. close() will throw an error for it.
	!cb_errno) {
      // Have to do the close in blocking mode since this object will
      // go away as soon as we return.
      set_blocking();
      close();
    }
    shutdown();
  } LEAVE;
}

string read (void|int length, void|int(0..1) not_all)
//! Read some (decrypted) data from the connection. Works like
//! @[Stdio.File.read].
//!
//! @note
//! I/O errors from both reading and writing might occur in blocking
//! mode.
{
  SSL3_DEBUG_MSG ("SSL.sslfile->read (%d, %d)\n", length, not_all);

  ENTER (0, 0) {
    // Only signal an error after an explicit close() call.
    if (close_state > 0) error ("Not open.\n");

    FIX_ERRNOS ({
	SSL3_DEBUG_MSG ("SSL.sslfile->read: Propagating old callback error: %s\n",
			strerror (local_errno));
	RETURN (0);
      }, 0);

    if (stream)
      if (not_all)
	RUN_MAYBE_BLOCKING (!sizeof (read_buffer), 0, 1,
			    RETURN (0));
      else
	RUN_MAYBE_BLOCKING (sizeof (read_buffer) < length || zero_type (length),
			    nonblocking_mode, 1,
			    RETURN (0));

    string res = read_buffer->get();
    if (!zero_type (length)) {
      read_buffer->add (res[length..]);
      res = res[..length-1];
    }

    SSL3_DEBUG_MSG ("SSL.sslfile->read: Read done, returning %d bytes "
		    "(%d still in buffer)",
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
//! during the handshake phase.
//!
//! @note
//! I/O errors from both reading and writing might occur in blocking
//! mode.
{
  if (sizeof (args))
    data = sprintf (arrayp (data) ? data * "" : data, @args);

  SSL3_DEBUG_MSG ("SSL.sslfile->write (%t[%d])\n", data, sizeof (data));

  ENTER (0, 0) {
    // Only signal an error after an explicit close() call.
    if (close_state > 0) error ("Not open.\n");

    FIX_ERRNOS ({
	SSL3_DEBUG_MSG ("SSL.sslfile->write: Propagating old callback error: %s\n",
			strerror (local_errno));
	RETURN (-1);
      }, 0);

    if (SSL_HANDSHAKING) {
      SSL3_DEBUG_MSG ("SSL.sslfile->write: "
		      "Still in handshake - cannot accept application data\n");
      RETURN (0);
    }

    // Take care of any old data first.
    if (!direct_write()) RETURN (-1);

    int written = 0;

    if (arrayp (data)) {
      int idx = 0, pos = 0;

      while (idx < sizeof (data) && !sizeof (write_buffer)) {
	int size = sizeof (data[idx]) - pos;
	if (size > SSL.Constants.PACKET_MAX_SIZE) {
	  int n = conn->send_streaming_data (data[idx][pos..]);
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
      while (written < sizeof (data) && !sizeof (write_buffer)) {
	int n = conn->send_streaming_data (data[written..]);
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
    // Only signal an error after an explicit close() call.
    if (close_state > 0) error ("Not open.\n");

    FIX_ERRNOS ({
	SSL3_DEBUG_MSG ("SSL.sslfile->renegotiate: "
			"Propagating old callback error: %s\n",
			strerror (local_errno));
	RETURN (0);
      }, 0);

    if (!stream) {
      SSL3_DEBUG_MSG ("SSL.sslfile->renegotiate: "
		      "Connection closed - simulating System.EPIPE\n");
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
{
  SSL3_DEBUG_MSG ("SSL.sslfile->set_nonblocking (%O, %O, %O, %O, %O, %O)\n%s",
		  read, write, close, read_oob, write_oob, accept,
		  "" || describe_backtrace (backtrace()));

  ENTER (0, 0) {
    // Only signal an error after an explicit close() call.
    if (close_state > 0) error ("Not open.\n");

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
{
  // Previously this function wrote the remaining write buffer to the
  // stream directly. But that can only be done safely if we implement
  // a lock here to wait for any nonblocking operations to complete,
  // and that could introduce a deadlock since there might be other
  // lock dependencies between the threads.

  SSL3_DEBUG_MSG ("SSL.sslfile->set_blocking()\n%s",
		  "" || describe_backtrace (backtrace()));

  ENTER (0, 0) {
    if (!close_state) {
      nonblocking_mode = 0;
      accept_callback = read_callback = write_callback = close_callback = 0;

      if (stream) {
	update_internal_state();
	stream->set_blocking();
      }
    }
  } LEAVE;
}

int errno()
//!
{
  // We don't check threads for most other query functions, but
  // looking at errno while doing I/O in another thread can't be done
  // safely.
  CHECK (0, 0);
  return local_errno ? local_errno : stream && stream->errno();
}

void set_alert_callback (function(object,int|object,string:void) alert)
//! Install a function that will be called when an alert packet is
//! received. It doesn't affect the callback mode - it's called both
//! from backends and from within normal function calls like @[read]
//! and @[write].
{
  SSL3_DEBUG_MSG ("SSL.sslfile->set_alert_callback (%O)\n", alert);
  CHECK (0, 0);
#ifdef DEBUG
  if (close_state == STREAM_UNINITIALIZED || !conn)
    error ("Doesn't have any connection.\n");
#endif
  conn->set_alert_callback (alert);
}

function(object,int|object,string:void) query_alert_callback()
//!
{
  return conn && conn->alert_callback;
}

void set_accept_callback (function(void|object,void|mixed:int) accept)
//! Install a function that will be called when the handshake is
//! finished and the connection is ready for use.
//!
//! the callback function will be called with the sslfile object
//! and the additional id arguments (set with @[set_id]).
//! 
//! @note
//! Like the read, write and close callbacks, installing this callback
//! implies callback mode, even after the handshake is done.
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
//!
{
  return accept_callback;
}

void set_read_callback (function(void|mixed,void|string:int) read)
//! Install a function to be called when data is available.
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
//!
{
  return read_callback;
}

void set_write_callback (function(void|mixed:int) write)
//! Install a function to be called when data can be written.
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
//!
{
  return write_callback;
}

void set_close_callback (function(void|mixed:int) close)
//! Install a function to be called when the connection is closed,
//! either normally or due to an error (use @[errno] to retrieve it).
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
//!
{
  return close_callback;
}

void set_id (mixed id)
//!
{
  SSL3_DEBUG_MSG ("SSL.sslfile->set_id (%O)\n", id);
  CHECK (0, 0);
  callback_id = id;
}

mixed query_id()
//!
{
  return callback_id;
}

void set_backend (Pike.Backend backend)
//! Set the backend used for the file callbacks.
{
  ENTER (0, 0) {
    // Only signal an error after an explicit close() call.
    if (close_state > 0) error ("Not open.\n");

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
{
  // Only signal an error after an explicit close() call.
  if (close_state > 0) error ("Not open.\n");
  return real_backend;
}

string query_address(int|void arg)
//!
{
  // Only signal an error after an explicit close() call.
  if (close_state > 0) error ("Not open.\n");
  return stream->query_address(arg);
}

int is_open()
//!
{
  return close_state <= 0 && stream && stream->is_open();
}

Stdio.File query_stream()
//! Return the underlying stream.
//!
//! @note
//! Avoid any temptation to do
//! @expr{destruct(sslfile_obj->query_stream())@}. That almost
//! certainly creates more problems than it solves.
{
  SSL3_DEBUG_MSG ("SSL.sslfile->query_stream(): Called from %s:%d\n",
		  backtrace()[-2][0], backtrace()[-2][1]);
  return stream;
}

SSL.connection query_connection()
//! Return the SSL connection object.
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


static void update_internal_state()
// Update the internal callbacks according to the current state.
{
  // When the local backend is used, callbacks are set explicitly
  // before it's started.
  if (stream->query_backend() != local_backend) {

    if (CALLBACK_MODE) {

      if (read_callback || close_callback || accept_callback || SSL_INTERNAL_TALK) {
	stream->set_read_callback (ssl_read_callback);
	stream->set_close_callback (ssl_close_callback);
	if (got_extra_read_call_out < 0) {
	  // Install it even if we're in a handshake. There might
	  // still be old data to read if we're renegotiating.
	  real_backend->call_out (ssl_read_callback, 0, 1, 0);
	  got_extra_read_call_out = 1;
	}
      }
      else {
	stream->set_read_callback (0);
	stream->set_close_callback (0);
	if (got_extra_read_call_out > 0) {
	  real_backend->remove_call_out (ssl_read_callback);
	  got_extra_read_call_out = -1;
	}
      }

      if (write_callback || sizeof (write_buffer))
	stream->set_write_callback (ssl_write_callback);
      else
	stream->set_write_callback (0);

      SSL3_DEBUG_MORE_MSG ("update_internal_state: "
			   "After handshake, callback mode [%O %O %O %O]\n",
			   stream->query_read_callback(),
			   stream->query_write_callback(),
			   stream->query_close_callback(),
			   got_extra_read_call_out);
      return;
    }

    // Not in callback mode. Can't install callbacks even though we'd
    // "need" to - have to cope with the local backend in each
    // operation instead.
    stream->set_read_callback (0);
    stream->set_close_callback (0);
    stream->set_write_callback (0);
    if (got_extra_read_call_out > 0) {
      real_backend->remove_call_out (ssl_read_callback);
      got_extra_read_call_out = -1;
    }

    SSL3_DEBUG_MORE_MSG ("update_internal_state: Not in callback mode [0 0 0 %O]\n",
			 got_extra_read_call_out);
  }
}

static int queue_write()
// Return 0 if the connection is still alive,
// 1 if it was closed politely, and -1 if it died unexpectedly
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

static int direct_write()
// Do a write directly. Something to write is assumed to exist (either
// in write_buffer or in the packet queue). Returns zero on error (as
// opposed to queue_write).
{
  if (!stream) {
    SSL3_DEBUG_MSG ("direct_write: "
		    "Connection already closed - simulating System.EPIPE\n");
    // If it was closed explicitly locally then close_state would be
    // set and we'd never get here, so we can report it as a remote
    // close.
    local_errno = System.EPIPE;
    return 0;
  }

  else {
    if (queue_write() < 0) {
      SSL3_DEBUG_MSG ("direct_write: "
		      "Connection closed abruptly - simulating System.EPIPE\n");
      local_errno = System.EPIPE;
      shutdown();
      return 0;
    }

    RUN_MAYBE_BLOCKING (sizeof (write_buffer) || SSL_INTERNAL_TALK,
			nonblocking_mode, SSL_INTERNAL_TALK,
			return 0;);
  }

  return 1;
}

private int call_close_callback()
{
  // errno() should return the error in the close callback - need to
  // propagate it here.
  FIX_ERRNOS (
    SSL3_DEBUG_MSG ("call_close_callback: Calling close callback %O (error %d)\n",
		    close_callback, cb_errno),
    SSL3_DEBUG_MSG ("call_close_callback: Calling close callback %O (read eof)\n",
		    close_callback)
  );
  close_state = CLOSE_CB_CALLED;
  return close_callback (callback_id);
}

static int ssl_read_callback (int called_from_real_backend, string input)
{
  SSL3_DEBUG_MSG ("ssl_read_callback (%O, %s): "
		  "nonblocking mode=%d, callback mode=%d%s%s\n",
		  called_from_real_backend,
		  input ? "string[" + sizeof (input) + "]" : "0 (queued extra call)",
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn && SSL_HANDSHAKING ? ", handshaking" : "",
		  conn && conn->closing ? ", closing (" + conn->closing + ")" : "");

  ENTER (1, called_from_real_backend) {
    int call_accept_cb;

    if (input) {
      int handshake_already_finished = conn->handshake_finished;
      string|int data = conn->got_data (input);

#ifdef SSL3_DEBUG_TRANSPORT
      werror ("ssl_read_callback: Got data: %O\n", data);
#endif

      int write_res;
      if (stringp(data) || (data > 0)) {
#ifdef DEBUG
	if (!stream)
	  error ("Got zapped stream in callback.\n");
#endif
	// got_data might have put more packets in the write queue.
	write_res = queue_write();
      }

      cb_errno = 0;

      if (stringp (data)) {
	SSL3_DEBUG_MSG ("ssl_read_callback: "
			"Got %d bytes of application data\n", sizeof (data));
	// Shouldn't come anything before the handshake is done, but anyway..
	read_buffer->add (data);
	if (!handshake_already_finished && conn->handshake_finished) {
	  SSL3_DEBUG_MSG ("ssl_read_callback: Handshake finished\n");
	  update_internal_state();
	  if (called_from_real_backend && accept_callback)
	    call_accept_cb = 1;
	}
      }

      else if (data < 0 || write_res < 0) {
	SSL3_DEBUG_MSG ("ssl_read_callback: "
			"Got abrupt remote close - simulating System.EPIPE\n");
	cb_errno = System.EPIPE;
      }

#ifdef SSL3_DEBUG
      // Don't use data > 0 here since we might have processed some
      // application data and a close in the same got_data call.
      if (conn->closing & 2)
	SSL3_DEBUG_MSG ("ssl_read_callback: Got close message\n");
#endif
    }

    else
      // This is another call that has been queued below through a
      // call out. That is necessary whenever we need to call several
      // user callbacks from the same invocation: We can't do anything
      // after calling a user callback, since the user is free to e.g.
      // remove the callbacks and "hand over" us to another thread and
      // do more I/O there while this one returns. We solve this
      // problem by queuing a call out to ourselves (with zero as
      // input) to continue.
      got_extra_read_call_out = 0;

    // Figure out what we need to do. call_accept_cb is already set
    // from above.
    int(0..1) call_read_cb =
      called_from_real_backend && read_callback && !!sizeof (read_buffer);
    int(0..1) do_close_stuff =
      !!(conn->closing & 2);

    if (cb_errno) {
      // An error changes everything..
      call_accept_cb = call_read_cb = 0;
      do_close_stuff = 1;
    }

    if (call_accept_cb + call_read_cb + do_close_stuff > 1) {
      // Need to do a call out to ourselves; see comment above.
#ifdef DEBUG
      if (!called_from_real_backend)
	error ("Internal confusion.\n");
      if (got_extra_read_call_out < 0)
	error ("Ended up in ssl_read_callback from real backend "
	       "when no callbacks are supposed to be installed.\n");
#endif
      if (!got_extra_read_call_out) {
	real_backend->call_out (ssl_read_callback, 0, 1, 0);
	got_extra_read_call_out = 1;
	SSL3_DEBUG_MSG ("ssl_read_callback: Too much to do (%O, %O, %O) - "
			"queued another call\n",
			call_accept_cb, call_read_cb, do_close_stuff);
      }
      else
	SSL3_DEBUG_MSG ("ssl_read_callback: Too much to do (%O, %O, %O) - "
			"another call already queued\n",
			call_accept_cb, call_read_cb, do_close_stuff);
    }

    else
      if (got_extra_read_call_out) {
	// Don't know if this actually can happen, but it's symmetric.
	if (got_extra_read_call_out > 0)
	  real_backend->remove_call_out (ssl_read_callback);
	got_extra_read_call_out = 0;
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
      update_internal_state();

      if (called_from_real_backend && close_callback && !close_state) {
	RESTORE;
	// Note that the callback should call close() (or free things
	// so that we get destructed) - there's no need for us to
	// schedule a shutdown after it.
	return call_close_callback();
      }

      if (cb_errno) {
	SSL3_DEBUG_MSG ("ssl_read_callback: Shutting down with error\n");
	shutdown();
	// Make sure the local backend exits after this, so that the
	// error isn't clobbered by later I/O.
	RESTORE;
	return -1;
      }
      else if (!sizeof (write_buffer)) {
	SSL3_DEBUG_MSG ("ssl_read_callback: "
			"Close messages exchanged - shutting down\n");
	shutdown();
      }
    }

  } LEAVE;
  return 0;
}

static int ssl_write_callback (int called_from_real_backend)
{
  SSL3_DEBUG_MSG ("ssl_write_callback (%O): "
		  "nonblocking mode=%d, callback mode=%d%s%s\n",
		  called_from_real_backend,
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn && SSL_HANDSHAKING ? ", handshaking" : "",
		  conn && conn->closing ? ", closing (" + conn->closing + ")" : "");

  int ret = 0;

  ENTER (1, called_from_real_backend) {
#ifdef DEBUG
    if (!stream)
      error ("Got zapped stream in callback.\n");
#endif

    if (got_extra_read_call_out) {
      // Not sure if this actually can happen, but handle it correctly
      // if it does - the call out would be out of order otherwise.
#ifdef DEBUG
      if (!called_from_real_backend)
	error ("RUN_MAYBE_BLOCKING is supposed "
	       "to handle got_extra_read_call_out first.\n");
      if (got_extra_read_call_out < 0)
	error ("Ended up in ssl_write_callback from real backend "
	       "when no callbacks are supposed to be installed.\n");
#endif
      real_backend->remove_call_out (ssl_read_callback);
      RESTORE;
      // ssl_read_callback will clear got_extra_read_call_out.
      return ssl_read_callback (called_from_real_backend, 0);
    }

  write_to_stream:
    do {
      if (sizeof (write_buffer)) {
	string output = write_buffer[0];
	int written = stream->write (output);

	if (written < 0
#if 0
#ifdef __NT__
	    // You don't want to know.. (Bug observed in Pike 0.6.132.)
	    && stream->errno() != 1
#endif
#endif
	   ) {
	  SSL3_DEBUG_MSG ("ssl_write_callback: Write failed: %s\n",
			  strerror (stream->errno()));
	  cb_errno = stream->errno();

	  // Make sure the local backend exits after this, so that the
	  // error isn't clobbered by later I/O.
	  ret = -1;

	  if ((<System.EPIPE, System.ECONNRESET>)[cb_errno]) {
	    SSL3_DEBUG_MSG ("ssl_write_callback: "
			    "Stream closed remotely - shutting down\n");
	    shutdown();
	  }

	  // Still try to call the write callback so that we propagate
	  // the error through the write() call it probably will do.
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

      if (int err = queue_write()) {
	if (err > 0) {
#ifdef DEBUG
	  if (!conn->closing)
	    error ("Expected a close to be sent or received\n");
#endif

	  if (!sizeof (write_buffer) &&
	      (conn->closing == 3 || close_state == NORMAL_CLOSE)) {
	    SSL3_DEBUG_MSG ("ssl_write_callback: %s - shutting down\n",
			    conn->closing == 3 ? "Close messages exchanged" :
			    "Close message sent");
	    shutdown();
	  }
	  else
	    SSL3_DEBUG_MSG ("ssl_write_callback: "
			    "Waiting for close message to be %s\n",
			    conn->closing == 1 ? "received" : "sent");

	  RESTORE;
	  return ret;
	}

	else {
	  SSL3_DEBUG_MSG ("ssl_write_callback: "
			  "Connection closed abruptly remotely - "
			  "simulating System.EPIPE\n");
	  cb_errno = System.EPIPE;
	  ret = -1;
	  shutdown();
	  break write_to_stream;
	}
      }
    } while (sizeof (write_buffer));

    if (called_from_real_backend && write_callback && !SSL_INTERNAL_TALK)
    {
      // errno() should return the error in the write callback - need
      // to propagate it here.
      FIX_ERRNOS (
	SSL3_DEBUG_MSG ("ssl_write_callback: Calling write callback %O (error %d)\n",
			write_callback, cb_errno),
	SSL3_DEBUG_MSG ("ssl_write_callback: Calling write callback %O\n",
			write_callback)
      );
      RESTORE;
      return write_callback (callback_id);
    }
  } LEAVE;
  return ret;
}

static int ssl_close_callback (int called_from_real_backend)
{
  SSL3_DEBUG_MSG ("ssl_close_callback (%O): "
		  "nonblocking mode=%d, callback mode=%d%s%s\n",
		  called_from_real_backend,
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn && SSL_HANDSHAKING ? ", handshaking" : "",
		  conn && conn->closing ? ", closing (" + conn->closing + ")" : "");

  ENTER (1, called_from_real_backend) {
#ifdef DEBUG
    if (!stream)
      error ("Got zapped stream in callback.\n");
#endif

    if (got_extra_read_call_out) {
      // Not sure if this actually can happen, but handle it correctly
      // if it does - the call out would be out of order otherwise.
#ifdef DEBUG
      if (!called_from_real_backend)
	error ("RUN_MAYBE_BLOCKING is supposed "
	       "to handle got_extra_read_call_out first.\n");
      if (got_extra_read_call_out < 0)
	error ("Ended up in ssl_close_callback from real backend "
	       "when no callbacks are supposed to be installed.\n");
#endif
      real_backend->remove_call_out (ssl_read_callback);
      RESTORE;
      // ssl_read_callback will clear got_extra_read_call_out.
      return ssl_read_callback (called_from_real_backend, 0);
    }

    // If we've arrived here due to an error, let it override any
    // older errno from an earlier callback.
    if (int new_errno = stream->errno()) {
      SSL3_DEBUG_MSG ("ssl_close_callback: Got error %d\n", new_errno);
      cb_errno = new_errno;
    }
#ifdef SSL3_DEBUG
    else if (cb_errno)
      SSL3_DEBUG_MSG ("ssl_close_callback: Propagating errno from another callback\n");
#endif

    if (!cb_errno) {
      if (conn->closing & 2) {
	// A proper close is handled in ssl_read_callback when we get
	// the close packet, so there's nothing to do here.
	SSL3_DEBUG_MSG ("ssl_close_callback: Clean close already handled\n");
	RESTORE;
	return 0;
      }

      // The remote end has closed the connection without sending a
      // close packet. Treat that as an error so that the caller can
      // detect truncation attacks.
      SSL3_DEBUG_MSG ("ssl_close_callback: Abrupt close - simulating System.EPIPE\n");
      cb_errno = System.EPIPE;
    }

    // Got an error.

    if (called_from_real_backend && close_callback && !close_state) {
      // Report the error using the close callback.
      RESTORE;
      // Note that the callback should call close() (or free things
      // so that we get destructed) - there's no need for us to
      // schedule a shutdown after it.
      return call_close_callback();
    }

    shutdown();
  } LEAVE;

  // Make sure the local backend exits after this, so that the error
  // isn't clobbered by later I/O.
  return -1;
}

#endif
