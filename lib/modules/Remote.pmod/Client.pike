#pike __REAL_VERSION__
#include "remote.h"
import ".";

//! Remote RPC Client.

int connected = 0;

//! @decl Remote.Connection con
//!
//! Connection to the @[Remote.Server].
object con;

function close_callback = 0;

//! Get a named object from the remote server.
object get(string name)
{
  if(!connected)
    error("Not connected\n");
  return con->get_named_object(name);
}

// This indirection function also serves the purpose of keeping a ref
// to this object in case the user doesn't.
void client_close_callback()
{
  if (close_callback) close_callback();
}

//! Connect to a @[Remote.Server].
//!
//! @param host
//! @param port
//!   Hostname and port for the @[Remote.Server].
//!
//! @param nice
//!   If set, inhibits throwing of errors from @[call_sync()].
//!
//! @param timeout
//!   Connection timeout in seconds.
//!
//! @param max_call_threads
//!   Maximum number of concurrent threads.
void create(string host, int port, void|int nice,
	    void|int timeout, void|int max_call_threads)
{
  con = Connection(nice, max_call_threads);
  if(!con->connect(host, port, timeout))
    error("Could not connect to server: %s\n", con->error_message());
  connected = 1;
  con->closed = 0;
  con->add_close_callback (client_close_callback);
}

//! Provide a named @[thing] to the @[Remote.Server].
//!
//! @param name
//!   Name to provide @[thing] under.
//!
//! @param thing
//!   Thing to provide.
void provide(string name, mixed thing)
{
  con->ctx->add(thing, name);
}

//! Set the callback to call when the connection is closed.
void set_close_callback(function f)
{
  close_callback = f;
}

//! Close the connection.
void close()
{
  con->close();
}

//! Check if the connection is closed.
int closed()
{
  return con->closed;
}

void destroy()
{
  catch (con->close());
}
