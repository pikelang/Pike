#pike __REAL_VERSION__

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

protected SSL.sslfd sslfd;
// The actual wrapped connection.

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
  if (sslfd) sslfd->close();
  sslfd = SSL.sslfd(stream, ctx, is_client, is_blocking,
		    min_version, max_version);
}

mixed get_server_names()
{
  return sslfd && sslfd->get_server_names();
}

//! @returns
//!   Returns peer certificate information, if any.
mapping get_peer_certificate_info()
{
  return sslfd && sslfd->get_peer_certificate_info();
}

//! @returns
//!   Returns the peer certificate chain, if any.
array get_peer_certificates()
{
  return sslfd && sslfd->get_peer_certificates();
}

//! Set the linger time on @[close()].
int(0..1) linger(int(-1..65535)|void seconds)
{
  return sslfd && sslfd->linger(seconds);
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
  return sslfd && sslfd->close();
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
  return sslfd && sslfd->shutdown();
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
  sslfd && call_out(sslfd->close, 0);
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
  if (!sslfd) error("Not open.\n");
  return sslfd->read(length, not_all);
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
  if (!sslfd) error("Not open.\n");
  return sslfd->write(data, @args);
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
  if (!sslfd) error("Not open.\n");
  return sslfd->renegotiate();
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
  sslfd && sslfd->set_callbacks(read, write, close,
				read_oob, write_oob, accept);
}

//! @returns
//!   Returns the currently set callbacks in the same order
//!   as the arguments to @[set_callbacks].
//!
//! @seealso
//!   @[set_callbacks], @[set_nonblocking]
array(function(mixed,void|string:int)) query_callbacks()
{
  return sslfd ? sslfd->query_callbacks() : allocate(6, UNDEFINED);
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
  sslfd && sslfd->set_nonblocking(read, write, close,
				  read_oob, write_oob, accept);
}

void set_nonblocking_keep_callbacks()
//! Set nonblocking mode like @[set_nonblocking], but don't alter any
//! callbacks.
//!
//! @seealso
//!   @[set_nonblocking], @[set_blocking], @[set_blocking_keep_callbacks]
{
  sslfd && sslfd->set_nonblocking_keep_callbacks();
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
  sslfd && sslfd->set_blocking();
}

void set_blocking_keep_callbacks()
//! Set blocking mode like @[set_blocking], but don't alter any
//! callbacks.
//!
//! @seealso
//!   @[set_blocking], @[set_nonblocking]
{
  sslfd && sslfd->set_blocking_keep_callbacks();
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
  return sslfd && sslfd->errno();
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
  sslfd && sslfd->set_alert_callback(alert);
}

function(object,int|object,string:void) query_alert_callback()
//! @returns
//!   Returns the current alert callback.
//!
//! @seealso
//!   @[set_alert_callback]
{
  return sslfd && sslfd->query_alert_callback();
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
  sslfd && sslfd->set_accept_callback(accept);
}

function(void|object,void|mixed:int) query_accept_callback()
//! @returns
//!   Returns the current accept callback.
//!
//! @seealso
//!   @[set_accept_callback]
{
  return sslfd && sslfd->query_accept_callback();
}

void set_read_callback (function(void|mixed,void|string:int) read)
//! Install a function to be called when data is available.
//!
//! @seealso
//!   @[query_read_callback], @[set_nonblocking], @[query_callbacks]
{
  sslfd && sslfd->set_read_callback(read);
}

function(void|mixed,void|string:int) query_read_callback()
//! @returns
//!   Returns the current read callback.
//!
//! @seealso
//!   @[set_read_callback], @[set_nonblocking], @[query_callbacks]
{
  return sslfd && sslfd->query_read_callback();
}

void set_write_callback (function(void|mixed:int) write)
//! Install a function to be called when data can be written.
//!
//! @seealso
//!   @[query_write_callback], @[set_nonblocking], @[query_callbacks]
{
  sslfd && sslfd->set_write_callback(write);
}

function(void|mixed:int) query_write_callback()
//! @returns
//!   Returns the current write callback.
//!
//! @seealso
//!   @[set_write_callback], @[set_nonblocking], @[query_callbacks]
{
  return sslfd && sslfd->query_write_callback();
}

void set_close_callback (function(void|mixed:int) close)
//! Install a function to be called when the connection is closed,
//! either normally or due to an error (use @[errno] to retrieve it).
//!
//! @seealso
//!   @[query_close_callback], @[set_nonblocking], @[query_callbacks]
{
  sslfd && sslfd->set_close_callback(close);
}

function(void|mixed:int) query_close_callback()
//! @returns
//!   Returns the current close callback.
//!
//! @seealso
//!   @[set_close_callback], @[set_nonblocking], @[query_callbacks]
{
  return sslfd && sslfd->query_close_callback();
}

void set_id (mixed id)
//! Set the value to be sent as the first argument to the
//! callbacks installed by @[set_callbacks].
//!
//! @seealso
//!   @[query_id]
{
  sslfd && sslfd->set_id(id);
}

mixed query_id()
//! @returns
//!   Returns the currently set id.
//!
//! @seealso
//!   @[set_id]
{
  return sslfd && sslfd->query_id();
}

void set_backend (Pike.Backend backend)
//! Set the backend used for the file callbacks.
//!
//! @seealso
//!   @[query_backend]
{
  sslfd && sslfd->set_backend(backend);
}

Pike.Backend query_backend()
//! Return the backend used for the file callbacks.
//!
//! @seealso
//!   @[set_backend]
{
  return sslfd && sslfd->query_backend();
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
  return sslfd && sslfd->query_address(arg);
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
  return sslfd && sslfd->is_open();
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
  return sslfd && sslfd->query_stream();
}

SSL.connection query_connection()
//! Return the SSL connection object.
//!
//! This returns the low-level @[SSL.connection] object.
{
  return sslfd && sslfd->query_connection();
}

SSL.context query_context()
//! Return the SSL context object.
{
  return sslfd && sslfd->query_context();
}

string _sprintf(int t) {
  return t=='O' && sprintf("SSL.sslfile(%O)", query_stream());
}

//! The next protocol chosen by the client during application layer
//! protocol negotiation (ALPN) or next protocol negotiation (NPN).
string `->next_protocol() {
  return sslfd && sslfd->next_protocol;
}

#else // constant(SSL.Cipher.CipherAlgorithm)
constant this_program_does_not_exist = 1;
#endif
