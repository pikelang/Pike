#pike __REAL_VERSION__
#require constant(SSL.Cipher)

#define Context .Context

//! Interface similar to @[Stdio.Port].

//!
inherit Stdio.Port : socket;

//! Context to use for the connections.
Context ctx;

protected ADT.Queue accept_queue = ADT.Queue();

//!
function(object, mixed|void:void) accept_callback;

//! @decl void finished_callback(SSL.File f, mixed|void id)
//!
//! SSL connection accept callback.
//!
//! @param f
//!   The @[File] that just finished negotiation.
//!
//! This function is installed as the @[File] accept callback by
//! @[ssl_callback()], and enqueues the newly negotiated @[File] on
//! the accept queue.
//!
//! If there has been an @[accept_callback] installed by @[bind()] or
//! @[listen_fd()], it will be called with all pending @[File]s on the
//! accept queue.
//!
//! If there's no @[accept_callback], then the @[File] will have to be
//! retrieved from the queue by calling @[accept()].
void finished_callback(object f, mixed|void id)
{
  accept_queue->put(f);
  while (accept_callback && sizeof(accept_queue))
  {
    accept_callback(f, id);
  }
}

//! Connection accept callback.
//!
//! This function is installed as the @[Stdio.Port] callback, and
//! accepts the connection and creates a corresponding @[File] with
//! @[finished_callback()] as the accept callback.
//!
//! @seealso
//!   @[bind()], @[finished_callback()]
void ssl_callback(mixed id)
{
  object f = socket_accept();
  if (f)
  {
    object ssl_fd = .File(f, ctx);
    ssl_fd->accept();
    ssl_fd->set_accept_callback(finished_callback);
  }
}

#if 0
void set_id(mixed id)
{
  error( "Not supported\n" );
}

mixed query_id()
{
  error( "Not supported\n" );
}
#endif

//! @decl int bind(int port, @
//!                function(SSL.File|void, mixed|void: int) callback, @
//!                string|void ip,@
//!                int|void share)
//!
//! Bind an SSL port.
//!
//! @param port
//!   Port number to bind.
//!
//! @param callback
//!   Callback to call when the SSL connection has been negotiated.
//!
//!   The callback is called with an @[File] as the first argument,
//!   and the id for the @[File] as the second.
//!
//!   If the @[callback] is @expr{0@} (zero), then negotiated @[File]s
//!   will be enqueued for later retrieval with @[accept()].
//!
//! @param ip
//!   Optional IP-number to bind.
//!
//! @param share
//!   If true, share the port with other processes
//!
//! @returns
//!   Returns @expr{1@} if binding of the port succeeded,
//!   and @expr{0@} (zero) on failure.
//!
//! @seealso
//!   @[Stdio.Port()->bind()], @[File()->set_accept_callback()],
//!   @[listen_fd()]
int bind(int port, function callback, string|void ip, int|void share)
{
  accept_callback = callback;
  return socket::bind(port, ssl_callback, ip, share);
}

//! @decl int listen_fd(int fd, @
//!                     function(File|void, mixed|void: int) callback)
//!
//! Set up listening for SSL connections on an already opened fd.
//!
//! @param fd
//!   File descriptor to listen on.
//!
//! @param callback
//!   Callback to call when the SSL connection has been negotiated.
//!
//!   The callback is called with an @[File] as the first argument,
//!   and the id for the @[File] as the second.
//!
//!   If the @[callback] is @expr{0@} (zero), then negotiated @[File]s
//!   will be enqueued for later retrieval with @[accept()].
//!
//! @returns
//!   Returns @expr{1@} if listening on the fd succeeded,
//!   and @expr{0@} (zero) on failure.
//!
//! @seealso
//!   @[Stdio.Port()->listen_fd()], @[File()->set_accept_callback()],
//!   @[bind()]
int listen_fd(int fd, function callback)
{
  accept_callback = callback;
  return socket::listen_fd(fd, ssl_callback);
}

//! Low-level accept.
//!
//! @seealso
//!   @[Stdio.Port()->accept()]
Stdio.File socket_accept()
{
  return socket::accept();
}

//! @decl File accept()
//!
//! Get the next pending @[File] from the @[accept_queue].
//!
//! @returns
//!   Returns the next pending @[File] if any, and @expr{0@} (zero) if
//!   there are none.
object accept()
{
  return accept_queue->get();
}

//! Create a new port for accepting SSL connections.
//!
//! @seealso
//!   @[bind()], @[listen_fd()]
protected void create(Context|void ctx)
{
#ifdef SSL3_DEBUG
  werror("SSL.Port->create\n");
#endif
  if (!ctx) ctx = Context();
  this::ctx = ctx;
}
