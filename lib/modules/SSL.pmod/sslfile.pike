#pike __REAL_VERSION__

/* $Id: sslfile.pike,v 1.74 2004/08/17 16:06:07 mast Exp $
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
#define SSL3_DEBUG_MSG(X...)						\
  werror ("[thr:" + this_thread()->id_number() +			\
	  "," + (stream ?						\
		 (stream->query_fd ?					\
		  "fd:" + stream->query_fd() :				\
		  replace (sprintf ("%O", stream), "%", "%%")) :	\
		 "disconnected") + "] " +				\
	  X)
#else
#define SSL3_DEBUG_MSG(X...) 0
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
static function(object, void|mixed:int) accept_callback;
static function(mixed,string:int) read_callback;
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

static int explicitly_closed = 1;
// 1 if we haven't initialized with a stream, 2 if the caller has
// requested a normal close, 3 if the caller has requested a clean
// close.

static int close_status;
// This is conn->closed after shutdown when conn has been destructed.

static int local_errno;
// If nonzero, override the errno on the stream with this.

static int cb_errno;
// Stores the errno from failed I/O in a callback so that the next
// visible I/O operations can report it properly.

static int got_extra_read_call_out;
// Nonzero when we have a call out to call_read_callback. See comment
// in ssl_read_callback. This is still set if we switch to
// non-callback mode so that the call out can be restored when
// switching back.

// This macro is used in all user called functions that can report I/O
// errors, both at the beginning and after ssl_write_callback calls.
#define FIX_ERRNOS(ERROR_RETURN) do {					\
    if (cb_errno) {							\
      /* Got a stored error from a previous callback that has failed. */ \
      local_errno = cb_errno;						\
      cb_errno = 0;							\
      {ERROR_RETURN;}							\
    }									\
    else								\
      local_errno = 0;							\
  } while (0)

#define CALLBACK_MODE							\
  (read_callback || write_callback || close_callback || accept_callback)

#define SSL_HANDSHAKING (!conn->handshake_finished)
#define SSL_CLOSING (conn->closing == 3 ? !sizeof (write_buffer) : conn->closing)
#define SSL_INTERNAL_TALK (SSL_HANDSHAKING || SSL_CLOSING)

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
	stream->set_read_callback ((ENABLE_READS) && ssl_read_callback); \
	stream->set_write_callback (sizeof (write_buffer) && ssl_write_callback); \
									\
	/* If we've received a close message then the other end can	\
	 * legitimately close the stream, so don't install our close	\
	 * callback. (Might still have to write our close message.) */	\
	stream->set_close_callback (conn->closing < 2 && ssl_close_callback); \
									\
	float|int(0..0) action =					\
	  local_backend (NONBLOCKING_MODE ? 0.0 : 0);			\
									\
	FIX_ERRNOS (							\
	  SSL3_DEBUG_MSG ("%s local backend ended with error\n",	\
			  NONBLOCKING_MODE ? "Nonblocking" : "Blocking"); \
	  if (stream) {							\
	    stream->set_backend (real_backend);				\
	    stream->set_id (1);						\
	    update_internal_state();					\
	  }								\
	  {ERROR_CODE;}							\
	  break run_maybe_blocking;					\
	);								\
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
    write_buffer = ({});
    read_buffer = String.Buffer();
    callback_id = 0;
    real_backend = stream->query_backend();
    local_backend = 0;
    explicitly_closed = 0;
    local_errno = cb_errno = 0;
    got_extra_read_call_out = 0;

    stream->set_read_callback (0);
    stream->set_write_callback (0);
    stream->set_close_callback (0);
    stream->set_id (1);

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
//! Returns zero if the connection already is closed or if there are
//! any I/O errors. @[errno] will give the details.
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
    if (explicitly_closed) {
      SSL3_DEBUG_MSG ("SSL.sslfile->close: Already closed\n");
      local_errno = System.EBADF;
      RETURN (0);
    }
    explicitly_closed = clean_close ? 3 : 2;

    FIX_ERRNOS (
      SSL3_DEBUG_MSG ("SSL.sslfile->close: Propagating old callback error\n");
      // Errors are thrown from close().
      error ("Failed to close SSL connection: %s\n", strerror (local_errno));
    );

    if (!stream)
      if (close_status == 3) {
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

    close_status = conn->closing;
    if (explicitly_closed == 2)
      // If we didn't request a clean close then we pretend to have
      // received a close message. According to the standard it's ok
      // anyway as long as the transport isn't used for anything else.
      close_status |= 2;

    SSL3_DEBUG_MSG ("SSL.sslfile->shutdown(): %s close\n",
		    close_status == 3 && !sizeof (write_buffer) ?
		    "clean" : "abrupt");

    if ((close_status & 2) && sizeof (conn->left_over || "")) {
#ifdef DEBUG
      werror ("Warning: Got buffered data after close in %O: %O%s\n", this,
	      conn->left_over[..99], sizeof (conn->left_over) > 100 ? "..." : "");
#endif
      read_buffer = String.Buffer (sizeof (conn->left_over));
      read_buffer->add (conn->left_over);
      explicitly_closed = 0;
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

    if (got_extra_read_call_out) {
      real_backend->remove_call_out (call_read_callback);
      got_extra_read_call_out = 0;
    }

    Stdio.File stream = global::stream;
    global::stream = 0;

    stream->set_read_callback (0);
    stream->set_write_callback (0);
    stream->set_close_callback (0);

    if (explicitly_closed == 2) {
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
    if (stream && !explicitly_closed) {
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
    if (explicitly_closed) error ("Not open.\n");

    FIX_ERRNOS (
      SSL3_DEBUG_MSG ("SSL.sslfile->read: Propagating old callback error: %s\n",
		      strerror (local_errno));
      RETURN (0);
    );

    if (got_extra_read_call_out) {
      // The queued read callback call is superseded now.
      real_backend->remove_call_out (call_read_callback);
      got_extra_read_call_out = 0;
    }

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
    if (explicitly_closed) error ("Not open.\n");

    FIX_ERRNOS (
      SSL3_DEBUG_MSG ("SSL.sslfile->write: Propagating old callback error: %s\n",
		      strerror (local_errno));
      RETURN (-1);
    );

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

	  SSL3_DEBUG_MSG ("SSL.sslfile->write: Queued data[%d][%d..%d] + data[%d..%d]\n",
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
    if (explicitly_closed) error ("Not open.\n");

    FIX_ERRNOS (
      SSL3_DEBUG_MSG ("SSL.sslfile->renegotiate: Propagating old callback error: %s\n",
		      strerror (local_errno));
      RETURN (0);
    );

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

void set_nonblocking (void|function(mixed,string:void) read,
		      void|function(void|mixed:void) write,
		      void|function(void|mixed:void) close,
		      void|function(void|mixed:void) read_oob,
		      void|function(void|mixed:void) write_oob,
		      void|function(void|mixed:void) accept)
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
    if (explicitly_closed) error ("Not open.\n");

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
    if (!explicitly_closed) {
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
  if (explicitly_closed == 1 || !conn) error ("Doesn't have any connection.\n");
#endif
  conn->set_alert_callback (alert);
}

function(object,int|object,string:void) query_alert_callback()
//!
{
  return conn && conn->alert_callback;
}

void set_accept_callback (function(object,void|mixed:void) accept)
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
    if (explicitly_closed == 1) error ("Doesn't have any connection.\n");
#endif
    accept_callback = accept;
    if (stream) update_internal_state();
  } LEAVE;
}

function(void|mixed:void) query_accept_callback()
//!
{
  return accept_callback;
}

void set_read_callback (function(mixed,string:void) read)
//! Install a function to be called when data is available.
{
  SSL3_DEBUG_MSG ("SSL.sslfile->set_read_callback (%O)\n", read);
  ENTER (0, 0) {
#ifdef DEBUG
    if (explicitly_closed == 1) error ("Doesn't have any connection.\n");
#endif
    read_callback = read;
    if (stream) update_internal_state();
  } LEAVE;
}

function(mixed,string:void) query_read_callback()
//!
{
  return read_callback;
}

void set_write_callback (function(void|mixed:void) write)
//! Install a function to be called when data can be written.
{
  SSL3_DEBUG_MSG ("SSL.sslfile->set_write_callback (%O)\n", write);
  ENTER (0, 0) {
#ifdef DEBUG
    if (explicitly_closed == 1) error ("Doesn't have any connection.\n");
#endif
    write_callback = write;
    if (stream) update_internal_state();
  } LEAVE;
}

function(void|mixed:void) query_write_callback()
//!
{
  return write_callback;
}

void set_close_callback (function(void|mixed:void) close)
//! Install a function to be called when the connection is closed,
//! either normally or due to an error.
{
  SSL3_DEBUG_MSG ("SSL.sslfile->set_close_callback (%O)\n", close);
  ENTER (0, 0) {
#ifdef DEBUG
    if (explicitly_closed == 1) error ("Doesn't have any connection.\n");
#endif
    close_callback = close;
    if (stream) update_internal_state();
  } LEAVE;
}

function(void|mixed:void) query_close_callback()
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
    if (explicitly_closed) error ("Not open.\n");

    if (stream) {
      real_backend = backend;
      if (stream->query_backend() != local_backend) {
	stream->set_backend (backend);

	if (got_extra_read_call_out) {
	  real_backend->remove_call_out (call_read_callback);
	  backend->call_out (call_read_callback, 0);
	}
      }
    }
    else real_backend = backend;
  } LEAVE;
}

Pike.Backend query_backend()
//! Return the backend used for the file callbacks.
{
  if (explicitly_closed) error ("Not open.\n");
  return real_backend;
}

string query_address(int|void arg)
//!
{
  // Only signal an error after an explicit close() call.
  if (explicitly_closed) error ("Not open.\n");
  return stream->query_address(arg);
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
	if (got_extra_read_call_out)
	  // Install it even if we're in a handshake. There might
	  // still be old data to read if we're renegotiating.
	  real_backend->call_out (call_read_callback, 0);
      }
      else {
	stream->set_read_callback (0);
	stream->set_close_callback (0);
	if (got_extra_read_call_out)
	  real_backend->remove_call_out (call_read_callback);
      }

      if (write_callback || sizeof (write_buffer))
	stream->set_write_callback (ssl_write_callback);
      else
	stream->set_write_callback (0);

#ifdef SSL3_DEBUG_MORE
      SSL3_DEBUG_MSG ("update_internal_state: "
		      "After handshake, callback mode [%O %O %O]\n",
		      stream->query_read_callback(),
		      stream->query_write_callback(),
		      stream->query_close_callback());
#endif
      return;
    }

    // Not in callback mode. Can't install callbacks even though we'd
    // "need" to - have to cope with the local backend in each
    // operation instead.
    stream->set_read_callback (0);
    stream->set_close_callback (0);
    stream->set_write_callback (0);
    if (got_extra_read_call_out)
      real_backend->remove_call_out (call_read_callback);

#ifdef SSL3_DEBUG_MORE
    SSL3_DEBUG_MSG ("update_internal_state: Not in callback mode [0 0 0]\n");
#endif
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
    // If it was closed explicitly locally then explicitly_closed
    // would be set and we'd never get here, so we can report it as a
    // remote close.
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

static int call_read_callback()
{
  SSL3_DEBUG_MSG ("call_read_callback(): "
		  "nonblocking mode=%d, callback mode=%d%s%s\n",
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn && SSL_HANDSHAKING ? ", handshaking" : "",
		  conn && SSL_CLOSING ? ", closing" : "");

  ENTER (1, 1) {
    got_extra_read_call_out = 0;

    if (read_callback && sizeof (read_buffer)) {
      string received = read_buffer->get();

      SSL3_DEBUG_MSG ("call_read_callback: Calling read callback %O "
		      "with %d bytes (%d still in buffer)\n",
		      read_callback, sizeof (received), sizeof (read_buffer));
      RESTORE;
      return read_callback (callback_id, received);
    }
  } LEAVE;
  return 0;
}

static int ssl_read_callback (int called_from_real_backend, string input)
{
  SSL3_DEBUG_MSG ("ssl_read_callback (%O, string[%d]): "
		  "nonblocking mode=%d, callback mode=%d%s%s\n",
		  called_from_real_backend, sizeof (input),
		  nonblocking_mode, !!(CALLBACK_MODE),
		  conn && SSL_HANDSHAKING ? ", handshaking" : "",
		  conn && SSL_CLOSING ? ", closing" : "");

  ENTER (1, called_from_real_backend) {
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

    if (stringp (data)) {
      SSL3_DEBUG_MSG ("ssl_read_callback: "
		      "Got %d bytes of application data\n", sizeof (data));

      // Shouldn't come anything before the handshake is done, but anyway..
      read_buffer->add (data);

      if (conn->handshake_finished) {
	if (!handshake_already_finished) {
	  SSL3_DEBUG_MSG ("ssl_read_callback: Handshake finished\n");
	  update_internal_state();

	  if (called_from_real_backend && accept_callback) {

	    // We can't do anything after calling a user callback,
	    // since the user is free to e.g. remove the callbacks and
	    // "hand over" us to another thread and do more I/O there
	    // while this one returns. That's problematic if we need
	    // to call both the accept and the read callbacks after
	    // each other. We solve it by queuing a call out for the
	    // read callback, and make sure that the call out follows
	    // the fate of the read callback.
	    if (read_callback && sizeof (read_buffer)) {

	      // Might be possible to already have the call out if
	      // there are two handshakes after each other.
	      if (!got_extra_read_call_out) {
		real_backend->call_out (call_read_callback, 0);
		// Don't store the call out id returned above since
		// that'd introduce a cyclic reference.
		got_extra_read_call_out = 1;
	      }
	      SSL3_DEBUG_MSG ("ssl_read_callback: "
			      "Queued call to read callback after accept callback\n");
	    }

	    SSL3_DEBUG_MSG ("ssl_read_callback: Calling accept callback %O\n",
			    accept_callback);
	    RESTORE;
	    return accept_callback (this, callback_id);
	  }
	}

	if (called_from_real_backend && read_callback && sizeof (read_buffer)) {
	  RESTORE;
	  return call_read_callback();
	}
      }
    }

    else if (data > 0) {
      if (conn->closing & 2) {
	SSL3_DEBUG_MSG ("ssl_read_callback: Got close message\n");
	update_internal_state();

	if (called_from_real_backend && close_callback && !explicitly_closed) {
	  SSL3_DEBUG_MSG ("ssl_read_callback: Calling close callback %O\n",
			  close_callback);
	  RESTORE;
	  return close_callback (callback_id);
	}
      }

      if (!sizeof (write_buffer)) {
	SSL3_DEBUG_MSG ("ssl_read_callback: "
			"Close messages exchanged - shutting down\n");
	shutdown();
      }
    }

    else if (data < 0 || write_res < 0) {
      SSL3_DEBUG_MSG ("ssl_read_callback: "
		      "Got abrupt remote close - simulating System.EPIPE\n");
      local_errno = cb_errno = System.EPIPE;
      shutdown();

      // Make sure the local backend exits after this, so that the
      // error isn't clobbered by later I/O.
      return -1;
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
		  conn && SSL_CLOSING ? ", closing" : "");

  int ret = 0;

  ENTER (1, called_from_real_backend) {
#ifdef DEBUG
    if (!stream)
      error ("Got zapped stream in callback.\n");
#endif

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
	  local_errno = cb_errno = stream->errno();

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
	      (conn->closing == 3 || explicitly_closed == 2)) {
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
	  local_errno = cb_errno = System.EPIPE;
	  ret = -1;
	  shutdown();
	  break write_to_stream;
	}
      }
    } while (sizeof (write_buffer));

    if (called_from_real_backend && write_callback && !SSL_INTERNAL_TALK)
    {
      SSL3_DEBUG_MSG ("ssl_write_callback: Calling write callback %O\n",
		      write_callback);
      RESTORE;
      int res = write_callback (callback_id);
      return ret == -1 ? -1 : res;
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
		  conn && SSL_CLOSING ? ", closing" : "");

  ENTER (1, called_from_real_backend) {
#ifdef DEBUG
    if (!stream)
      error ("Got zapped stream in callback.\n");
#endif

    // Always signal an error and do a blunt shutdown here. That since
    // the connection has always been closed abruptly if we arrive
    // here (a proper close arrives in the read callback). Thus a
    // close callback should be called here - but we shouldn't hide
    // truncation attacks (hence the errno), let the application decide
    // if the error should be shown.
    if (cb_errno)
      SSL3_DEBUG_MSG ("ssl_close_callback: Got errno from another callback\n");
    else {
      SSL3_DEBUG_MSG ("ssl_close_callback: Abrupt close - simulating System.EPIPE\n");
      function(void|mixed:int) old_close_callback = close_callback;
      close_callback = 0;
      if(old_close_callback)
        old_close_callback (callback_id);
      cb_errno = System.EPIPE;
    }
    shutdown();

    // What about a nonblocking close? If we don't throw anything here
    // then the error will go unnoticed for sure. But then again, if
    // someone makes a nonblocking close that's probably what's wanted.
  } LEAVE;

  // Make sure the local backend exits after this, so that the error
  // isn't clobbered by later I/O.
  return -1;
}

#endif
