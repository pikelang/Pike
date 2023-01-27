#pike __REAL_VERSION__
#require constant(SSL.Cipher)

#define Context .Context

//! Interface similar to @[Stdio.Port].

//!
inherit Stdio.Port : socket;

//! Function called to create the @[Context] object for this @[Port].
//!
//! By overriding this function the setup of certificates, etc
//! for the port can be delayed until the first access to the port.
//!
//! @returns
//!   Returns the @[Context] to be used with this @[Port].
Context context_factory()
{
  return Context();
}

protected Context _ctx;

//! @[Context] to use for the connections.
//!
//! @note
//!   The @[Context] is created (by calling @[context_factory()])
//!   on first access to the variable.
Context `ctx()
{
  if (_ctx) return _ctx;
  return _ctx = context_factory();
}

void `ctx=(Context c)
{
  _ctx = c;
}

protected ADT.Queue accept_queue = ADT.Queue();

//!
function(mixed|void:void) accept_callback;

protected void call_accept_callback()
{
  function cb;
  while ((cb = accept_callback) && sizeof(accept_queue)) {
    mixed err = catch {
        cb(_id);
      };
    if (err) {
      master()->handle_error(err);
    }
  }
}

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
  call_accept_callback();
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
//!                function(SSL.File|void, mixed|void: int)|void callback, @
//!                string|void ip,@
//!                int|void reuse_port)
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
//! @param reuse_port
//!   If true, enable SO_REUSEPORT if the OS supports it.
//!
//! @returns
//!   Returns @expr{1@} if binding of the port succeeded,
//!   and @expr{0@} (zero) on failure.
//!
//! @seealso
//!   @[Stdio.Port()->bind()], @[File()->set_accept_callback()],
//!   @[listen_fd()]
int bind(int port, function|void callback, string|void ip, int|void reuse_port)
{
  accept_callback = callback;
  return socket::bind(port, callback && ssl_callback, ip, reuse_port);
}

//! @decl int listen_fd(int fd, @
//!                     function(File|void, mixed|void: int)|void callback)
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
int listen_fd(int fd, function|void callback)
{
  accept_callback = callback;
  return socket::listen_fd(fd, callback && ssl_callback);
}

//! Set the accept callback.
void set_accept_callback(function|void accept_callback)
{
  this::accept_callback = accept_callback;
  socket::set_accept_callback(ssl_callback);
  call_out(call_accept_callback, 0);
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
  if (!socket::_accept_callback) {
    // Enable the callback on first call in order to allow
    // late initialization of the context.
    socket::set_accept_callback(ssl_callback);
  }
  return accept_queue->get();
}

//! Create a new port for accepting SSL connections.
//!
//! @param ctx
//!   @[Context] to be used with this @[Port].
//!
//!   If left out, it will be created on demand on first access
//!   by calling @[context_factory()].
//!
//! @seealso
//!   @[bind()], @[listen_fd()]
protected void create(Context|void ctx)
{
#ifdef SSL3_DEBUG
  werror("SSL.Port->create\n");
#endif
  if (ctx)
    this::ctx = ctx;
}
