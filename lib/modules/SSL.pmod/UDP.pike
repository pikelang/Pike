#pike __REAL_VERSION__
#require constant(SSL.Cipher)

#define Context .Context

import .Constants;

//! Interface similar to @[Stdio.UDP].

//!
inherit Stdio.UDP : udp;

//! Function called to create the @[Context] object for this @[UDP].
//!
//! By overriding this function the setup of certificates, etc
//! for the port can be delayed until the first access to the port.
//!
//! @returns
//!   Returns the @[Context] to be used with this @[UDP].
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

//! Connection accept callback.
//!
//! This function is installed as the @[Stdio.UDP] callback, and
//! accepts the connection and creates a corresponding @[File] with
//! @[finished_callback()] as the accept callback.
//!
//! @seealso
//!   @[bind()], @[finished_callback()]
void ssl_callback(mixed id)
{
#if 0
  object f = socket_accept();
  if (f)
  {
    object ssl_fd = .File(f, ctx);
    ssl_fd->accept();
    ssl_fd->set_accept_callback(finished_callback);
  }
#endif
}

//! Create a new port for accepting DTLS/UDP connections.
//!
//! @param ctx
//!   @[Context] to be used with this @[UDP].
//!
//!   If left out, it will be created on demand on first access
//!   by calling @[context_factory()].
protected void create(Context|void ctx)
{
#ifdef SSL3_DEBUG
  werror("SSL.UDP->create\n");
#endif
  if (ctx)
    this::ctx = ctx;
}

//! Currently active @[Connection]s and operation mode.
//!
//! @mixed
//!   @type DTLSClientConnection
//!     The connection to a remote DTLS server.
//!     Indicates DTLS client mode.
//!     Initiated via a call to @[connect()].
//!   @type mapping(string:.DTLSServerConnection)
//!     Lookup from remote ip and port to @[DTLSServerConnection].
//!     Indicates DTLS server mode.
//!     Initiated via a call to @[bind()].
//!   @type zero
//!     Uninitialized port.
//! @endmixed
//!
//! @seealso
//!   @[bind()], @[connect()]
mapping(string:.DTLSServerConnection)|.DTLSClientConnection connections;

protected ADT.Queue read_buffer = ADT.Queue();

int(0..1) close()
{
  connections = 0;
  read_buffer = ADT.Queue();
  return ::close();
}

//! Establish a DTLS "connection".
//!
//! Configures the UDP port as a DTLS client connecting to the
//! specified host and port.
//!
//! @seealso
//!   @[bind()]
.Session connect(string address, int|string port, .Session|void session)
{
  if (!udp::connect(address, port)) return 0;

  if (!session) {
    session = .Session();
  }

  int mtu = query_mtu();
  if (session->max_packet_size > mtu) session->max_packet_size = mtu;

  connections = .DTLSClientConnection(ctx, address, session);
  session = connections->session;

  udp::set_read_callback(got_packet);
  // udp::set_write_callback(ssl_write_callback);

  // FIXME: Wait for handshake to finish in blocking mode.

  return session;
}

//! Bind a UDP port for receiving DTLS.
//!
//! Binds a UDP port and sets it up as a DTLS server.
//!
//! @seealso
//!   @[connect()]
this_program bind(int|string port, string|void address,
		  string|int(0..1) no_reuseaddr)
{
  if (!::bind(port, address, no_reuseaddr)) return 0;

  connections = ([]);

  udp::set_read_callback(got_packet);
  return this;
}

//! Query the Maximum Tranfer Unit.
//!
//! @returns
//!   Returns the maximum number of bytes of application data
//!   that may be sent in a single @[send()] call.
//!
//! @seealso
//!   @[send()]
int query_mtu()
{
  int val = udp::query_mtu();

  int header_size = 1+2+2+6+2+2;

  if (val > header_size) return val - header_size;

  // Guess a common value.
  return 1500 - 40 - 8 - header_size;
}

//! Check whether to accept connections from a specific ip and port.
//!
//! The default implementation returns true.
//!
//! Override to implement more interesting policies.
protected int(0..1) accept_connectionp(string(8bit) ip, int(16bit) port)
{
  return 1;
}

protected void send_packet()
{
  Stdio.Buffer buf = Stdio.Buffer();
  int remaining;

  if (mappingp(connections)) {
    foreach(connections; string other; .Connection conn) {
      if (conn->to_write(buf) == 1) {
	array(string) a = other/"\0";
	udp::send(a[0], (int)a[1], buf->get());
	remaining += conn->query_write_queue_size();
      }
    }
  } else {
    .Connection conn = connections;
    if (conn->to_write(buf) == 1) {
      udp::send("", 0, buf->get());
      remaining = conn->query_write_queue_size();
    }
  }
  // if (!remaining) udp::set_write_callback(0);
}

protected void got_packet(mapping(string(8bit):int|string(8bit)) udp_packet)
{
  .Connection conn;
  if (mappingp(connections)) {
    // DTLS server mode.
    string(8bit) key = sprintf("%s\0%d", udp_packet->ip, udp_packet->port);
    conn = connections[key];
    if (!conn) {
      // Check whether it looks like a client hello.
      int content_type, version, epoch, len, message_type;
      if ((sscanf(udp_packet->data, "%1c%2c%2c%6*c%2c%1c",
		  content_type, version, epoch, len,
		  message_type) < 6) ||
	  (content_type != PACKET_handshake) ||
	  ((version & 0xff00) != 0xfe00) ||
	  epoch ||
	  (len < 32) ||
	  (message_type != HANDSHAKE_client_hello)) {
	// Not a DTLS client hello.
	return;
      }
      if (!accept_connectionp(udp_packet->ip, udp_packet->port)) {
	return;
      }
      conn = connections[key] = .DTLSServerConnection(ctx);
    }
  } else if (!connections) {
    // Uninitialized.
    return;
  } else {
    // DTLS client mode.
    conn = connections;
  }

  string(8bit)|int(-1..1) res = conn->got_data(udp_packet->data);
  if (stringp(res) && sizeof(res)) {
    read_buffer->put(udp_packet + ([ "data":res ]));
  }

#if 0
  // FIXME: Handle close and errors.
  if (conn->query_write_queue_size()) {
    udp::set_write_callback(send_packet);
  }
#endif
}

int(0..1) wait(int|float timeout)
{
  if (sizeof(read_buffer)) return 1;
  while (!sizeof(read_buffer)) {
    if (timeout <= 0.0) return 0;
    int start = gethrtime();
    if (!udp::wait(timeout)) return 0;
    mapping(string(8bit):int|string(8bit)) udp_packet = udp::read();
    got_packet(udp_packet);
    timeout -= (gethrtime()-start)/1000000.0;
  }
  return 1;
}

mapping(string(8bit):int|string(8bit)) read(int|void mode)
{
  if (mode & MSG_OOB) return 0;	// OOB not supported.
  while (!sizeof(read_buffer)) {
    mapping(string(8bit):int|string(8bit)) udp_packet = udp::read();
    if (!udp_packet) return 0;
    got_packet(udp_packet);
  }

  if (mode & 2) {
    return read_buffer->peek();
  }
  return read_buffer->get();
}

int udp_send(mixed ... args)
{
  return udp::send(@args);
}

int send(string(8bit) to, int|string(8bit) port, string(8bit) message,
	 int|void flags)
{
  .Connection conn;
  if (mappingp(connections)) {
    // DTLS server mode.
    string(8bit) key = sprintf("%s\0%x", to, port);
    conn = connections[key];
    if (!conn) return 0;
  } else if (!connections) {
    // Uninitialized.
    return 0;
  } else {
    // DTLS client mode.
    conn = connections;
  }

  // FIXME: Consider nonblocking mode.

  return conn->send_streaming_data(message);
}
