#pike __REAL_VERSION__
#require constant(SSL.Cipher)

//! Dummy HTTPS server/client

#ifndef PORT
#define PORT 25678
#endif

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

import Stdio;

#ifndef HTTPS_CLIENT
SSL.sslport port;

void my_accept_callback(object f)
{
  werror("Accept!\n");
  conn(port->accept());
}
#endif

class conn {
  import Stdio;

  object sslfile;

  string message =
    "HTTP/1.0 200 Ok\r\n"
    "Connection: close\r\n"
    "Content-Length: 132\r\n"
    "Content-Type: text/html; charset=ISO-8859-1\r\n"
    "Date: Thu, 01 Jan 1970 00:00:01 GMT\r\n"
    "Server: Bare-Bones\r\n"
    "\r\n"
    "<html><head><title>SSL-3 server</title></head>\n"
    "<body><h1>This is a minimal SSL-3 http server</h1>\n"
    "<hr><it>/nisse</it></body></html>\n";
  int index = 0;

  void write_callback()
  {
    if (index < sizeof(message))
    {
      int written = sslfile->write(message[index..]);
      if (written > 0)
	index += written;
      else
	sslfile->close();
    }
    if (index == sizeof(message))
      sslfile->close();
  }
  
  void read_callback(mixed id, string data)
  {
    SSL3_DEBUG_MSG("Received: '" + data + "'\n");
    sslfile->set_write_callback(write_callback);
  }

  protected void create(object f)
  {
    sslfile = f;
    sslfile->set_nonblocking(read_callback, 0, 0);
  }
}

class no_random {
  object arcfour = Crypto.Arcfour();
  
  protected void create(string|void secret)
  {
    if (!secret)
      secret = sprintf("%s%4c", random_string(32), time());
    arcfour->set_encrypt_key(Crypto.SHA256.hash(secret));
    read(1000);
  }

  string read(int size)
  {
    return arcfour->crypt( "\021"*size );
  }
}

/* PKCS#1 Private key structure:

RSAPrivateKey ::= SEQUENCE {
  version Version,
  modulus INTEGER, -- n
  publicExponent INTEGER, -- e
  privateExponent INTEGER, -- d
  prime1 INTEGER, -- p
  prime2 INTEGER, -- q
  exponent1 INTEGER, -- d mod (p-1)
  exponent2 INTEGER, -- d mod (q-1)
  coefficient INTEGER -- (inverse of q) mod p }

Version ::= INTEGER

*/

class client
{
  constant request =
    "HEAD / HTTP/1.0\r\n"
    "Host: localhost:" + PORT + "\r\n"
    "\r\n";

  SSL.sslfile ssl;
  int sent;
  void write_cb()
  {
    int bytes = ssl->write(request[sent..]);
    if (bytes > 0) {
      sent += bytes;
    } else if (sent < 0) {
      werror("Failed to write data: %s\n", strerror(ssl->errno()));
      exit(17);
    }
    if (sent == sizeof(request)) {
      ssl->set_write_callback(UNDEFINED);
    }
  }
  void got_data(mixed ignored, string data)
  {
    werror("Data: %O\n", data);
  }
  void con_closed()
  {
    werror("Connection closed.\n");
    exit(0);
  }

  protected void create(Stdio.File con)
  {
    SSL.context ctx = SSL.context();
    ctx->random = no_random()->read;
    // Make sure all cipher suites are available.
    ctx->preferred_suites = ctx->get_suites(-1, 2);
    werror("Starting\n");
    ssl = SSL.sslfile(con, ctx, 1);
    ssl->set_nonblocking(got_data, write_cb, con_closed);
  }
}

int main()
{
#ifdef HTTPS_CLIENT
  Stdio.File con = Stdio.File();
  if (!con->connect("127.0.0.1", PORT)) {
    werror("Failed to connect to server: %s\n", strerror(con->errno()));
    return 17;
  }
  client(con);
  return -17;
#else
  SSL.context ctx = SSL.context();

  Crypto.Sign key;
  string certificate;

  key = Crypto.RSA()->generate_key(1024);
  certificate =
    Standards.X509.make_selfsigned_certificate(key, 3600*4, ([
						 "organizationName" : "Test",
						 "commonName" : "*",
					       ]));

  ctx->add_cert(key, ({ certificate }), ({ "*" }));

  key = Crypto.DSA()->generate_key(1024, 160);
  certificate =
    Standards.X509.make_selfsigned_certificate(key, 3600*4, ([
						 "organizationName" : "Test",
						 "commonName" : "*",
					       ]));
  ctx->add_cert(key, ({ certificate }));

#if constant(Crypto.ECC.Curve)
  key = Crypto.ECC.SECP_521R1.ECDSA()->generate_key();
  certificate =
    Standards.X509.make_selfsigned_certificate(key, 3600*4, ([
						 "organizationName" : "Test",
						 "commonName" : "*",
					       ]));
  ctx->add_cert(key, ({ certificate }));
#endif

  // Make sure all cipher suites are available.
  ctx->preferred_suites = ctx->get_suites(-1, 2);
  SSL3_DEBUG_MSG("Cipher suites:\n%s",
                 .Constants.fmt_cipher_suites(ctx->preferred_suites));

  SSL3_DEBUG_MSG("Certs:\n%O\n", ctx->cert_pairs);

  ctx->random = no_random()->read;

  port = SSL.sslport(ctx);

  werror("Starting\n");
  if (!port->bind(PORT, my_accept_callback))
  {
    perror("");
    return 17;
  }
  else {
    werror("Listening on port %d.\n", PORT);
    return -17;
  }
#endif
}
