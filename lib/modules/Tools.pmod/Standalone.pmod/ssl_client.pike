#pike __REAL_VERSION__
#require constant(SSL.Cipher)

//! SSL/TLS client

constant description = "SSL/TLS client.";

class MyContext
{
  inherit SSL.Context;

  SSL.Alert alert_factory(SSL.Connection con,
			  int level, int description,
			  SSL.Constants.ProtocolVersion version,
			  string|void message)
  {
    if (message && description) {
      werror("ALERT [%s: %d:%d]: %s",
	     SSL.Constants.fmt_version(version),
	     level, description, message);
    }
    return ::alert_factory(con, level, description, version, message);
  }
}

class Proxy(Stdio.File|SSL.File in,
	    Stdio.File|SSL.File out)
{
  protected void create()
  {
    Stdio.Buffer buf = Stdio.Buffer();

    in->set_nonblocking();
    out->set_nonblocking();

    array(Stdio.Buffer) bufs = in->query_buffer_mode();
    bufs[0] = buf;
    in->set_buffer_mode(@bufs);
    in->set_close_callback(lambda() {
			     werror("Connection closed.\n");
			     out->close("w");
			   });
    bufs = out->query_buffer_mode();
    bufs[1] = buf;
    out->set_buffer_mode(@bufs);
  }
}

class Options
{
  inherit Arg.Options;

  constant help_pre = "pike -x ssl_client";

  Opt host = HasOpt("-h")|HasOpt("--host")|Default("127.0.0.1");
  Opt port = HasOpt("-p")|HasOpt("--port")|Default(443);
}

int main(int argc, array(string) argv)
{
  Options options = Options(argv);

  Stdio.File con = Stdio.File();
  if (!con->connect(options->host, options->port))
    exit(1, "Failed to connect to server: %s.\n", strerror(con->errno()));

  SSL.Context ctx = MyContext();
  // Make sure all cipher suites are available.
  ctx->preferred_suites = ctx->get_suites(-1, 2);
  werror("Starting\n");
  SSL.File ssl = SSL.File(con, ctx);
  ssl->connect();

  while (string s = Stdio.stdin->gets()) {
    ssl->write(s);
  }
  return -1;
}
