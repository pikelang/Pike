#pike 7.8

#if constant(SSL.Cipher.CipherAlgorithm)

//! Interface similar to @[Stdio.Port].

//!
inherit Stdio.Port : socket;

//!
inherit .context;

//!
inherit ADT.Queue : accept_queue;

constant sslfile = SSL.sslfile;

//!
function(object, mixed|void:void) accept_callback;

//! @decl void finished_callback(SSL.sslfile f, mixed|void id)
//!
//! SSL connection accept callback.
//!
//! @param f
//!   The @[SSL.sslfile] that just finished negotiation.
//!
//! This function is installed as the @[SSL.sslfile] accept
//! callback by @[ssl_callback()], and enqueues the newly
//! negotiated @[SSL.sslfile] on the @[accept_queue].
//!
//! If there has been an @[accept_callback] installed by
//! @[bind()] or @[listen_fd()], it will be called with
//! all pending @[SSL.sslfile]s on the @[accept_queue].
//!
//! If there's no @[accept_callback], then the @[SSL.sslfile]
//! will have to be retrieved from the queue by calling @[accept()].
void finished_callback(object f, mixed|void id)
{
  accept_queue::put(f);
  while (accept_callback && !accept_queue::is_empty())
  {
    accept_callback(f, id);
  }
}

//! Connection accept callback.
//!
//! This function is installed as the @[Stdio.Port] callback,
//! and accepts the connection and creates a corresponding
//! @[SSL.sslfile] with @[finished_callback()] as the accept
//! callback.
//!
//! @seealso
//!   @[bind()], @[finished_callback()]
void ssl_callback(mixed id)
{
  object f = id->socket_accept();
  if (f)
  {
    sslfile(f, this)->set_accept_callback(finished_callback);
  }
}

//! @decl int bind(int port, @
//!                function(SSL.sslfile|void, mixed|void: int) callback, @
//!                string|void ip)
//!
//! Bind an SSL port.
//!
//! @param port
//!   Port number to bind.
//!
//! @param callback
//!   Callback to call when the SSL connection has been negotiated.
//!
//!   The callback is called with an @[SSL.sslfile] as the first argument,
//!   and the id for the @[SSL.sslfile] as the second.
//!
//!   If the @[callback] is @expr{0@} (zero), then negotiated
//!   @[SSL.sslfile]s will be enqueued for later retrieval with
//!   @[accept()].
//!
//! @param ip
//!   Optional IP-number to bind.
//!
//! @returns
//!   Returns @expr{1@} if binding of the port succeeded,
//!   and @expr{0@} (zero) on failure.
//!
//! @seealso
//!   @[Stdio.Port()->bind()], @[SSL.sslfile()->set_accept_callback()],
//!   @[listen_fd()]
int bind(int port, function callback, string|void ip)
{
  accept_callback = callback;
  return socket::bind(port, ssl_callback, ip);
}

//! @decl int listen_fd(int fd, @
//!                     function(SSL.sslfile|void, mixed|void: int) callback)
//!
//! Set up listening for SSL connections on an already opened fd.
//!
//! @param fd
//!   File descriptor to listen on.
//!
//! @param callback
//!   Callback to call when the SSL connection has been negotiated.
//!
//!   The callback is called with an @[SSL.sslfile] as the first argument,
//!   and the id for the @[SSL.sslfile] as the second.
//!
//!   If the @[callback] is @expr{0@} (zero), then negotiated
//!   @[SSL.sslfile]s will be enqueued for later retrieval with
//!   @[accept()].
//!
//! @returns
//!   Returns @expr{1@} if listening on the fd succeeded,
//!   and @expr{0@} (zero) on failure.
//!
//! @seealso
//!   @[Stdio.Port()->listen_fd()], @[SSL.sslfile()->set_accept_callback()],
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

//! @decl SSL.sslfile accept()
//!
//! Get the next pending @[SSL.sslfile] from the @[accept_queue].
//!
//! @returns
//!   Returns the next pending @[SSL.sslfile] if any,
//!   and @expr{0@} (zero) if there are none.
object accept()
{
  return accept_queue::get();
}

//! Create a new port for accepting SSL connections.
//!
//! @seealso
//!   @[bind()], @[listen_fd()]
void create()
{
#ifdef SSL3_DEBUG
  werror("SSL.sslport->create\n");
#endif
  context::create();
  accept_queue::create();
  set_id(this);
}

#else // constant(SSL.Cipher.CipherAlgorithm)
constant this_program_does_not_exist = 1;
#endif
