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
inherit SSL.sslport;

protected void create()
{
  SSL3_DEBUG_MSG("https->create\n");
  sslport::create();
}

void my_accept_callback(object f)
{
  werror("Accept!\n");
  conn(accept());
}

protected string fmt_cipher_suites(array(int) s)
{
  String.Buffer b = String.Buffer();
  mapping(int:string) ciphers = ([]);
  foreach([array(string)]indices(SSL.Constants), string id)
    if( has_prefix(id, "SSL_") || has_prefix(id, "TLS_") ||
	has_prefix(id, "SSL2_") )
      ciphers[SSL.Constants[id]] = id;
  foreach(s, int c)
    b->sprintf("   %-6d: %010x: %s\n",
	       c, cipher_suite_sort_key(c), ciphers[c]||"unknown");
  return (string)b;
}
#endif

string my_certificate = MIME.decode_base64(
  "MIIBxDCCAW4CAQAwDQYJKoZIhvcNAQEEBQAwbTELMAkGA1UEBhMCREUxEzARBgNV\n"
  "BAgTClRodWVyaW5nZW4xEDAOBgNVBAcTB0lsbWVuYXUxEzARBgNVBAoTClRVIEls\n"
  "bWVuYXUxDDAKBgNVBAsTA1BNSTEUMBIGA1UEAxMLZGVtbyBzZXJ2ZXIwHhcNOTYw\n"
  "NDMwMDUzNjU4WhcNOTYwNTMwMDUzNjU5WjBtMQswCQYDVQQGEwJERTETMBEGA1UE\n"
  "CBMKVGh1ZXJpbmdlbjEQMA4GA1UEBxMHSWxtZW5hdTETMBEGA1UEChMKVFUgSWxt\n"
  "ZW5hdTEMMAoGA1UECxMDUE1JMRQwEgYDVQQDEwtkZW1vIHNlcnZlcjBcMA0GCSqG\n"
  "SIb3DQEBAQUAA0sAMEgCQQDBB6T7bGJhRhRSpDESxk6FKh3iKKrpn4KcDtFM0W6s\n"
  "16QSPz6J0Z2a00lDxudwhJfQFkarJ2w44Gdl/8b+de37AgMBAAEwDQYJKoZIhvcN\n"
  "AQEEBQADQQB5O9VOLqt28vjLBuSP1De92uAiLURwg41idH8qXxmylD39UE/YtHnf\n"
  "bC6QS0pqetnZpQj1yEsjRTeVfuRfANGw\n");

string my_key = MIME.decode_base64(
  "MIIBOwIBAAJBAMEHpPtsYmFGFFKkMRLGToUqHeIoqumfgpwO0UzRbqzXpBI/PonR\n"
  "nZrTSUPG53CEl9AWRqsnbDjgZ2X/xv517fsCAwEAAQJBALzUbJmkQm1kL9dUVclH\n"
  "A2MTe15VaDTY3N0rRaZ/LmSXb3laiOgBnrFBCz+VRIi88go3wQ3PKLD8eQ5to+SB\n"
  "oWECIQDrmq//unoW1+/+D3JQMGC1KT4HJprhfxBsEoNrmyIhSwIhANG9c0bdpJse\n"
  "VJA0y6nxLeB9pyoGWNZrAB4636jTOigRAiBhLQlAqhJnT6N+H7LfnkSVFDCwVFz3\n"
  "eygz2yL3hCH8pwIhAKE6vEHuodmoYCMWorT5tGWM0hLpHCN/z3Btm38BGQSxAiAz\n"
  "jwsOclu4b+H8zopfzpAaoB8xMcbs0heN+GNNI0h/dQ==\n");

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
      secret = sprintf("Foo!%4c", time());
    arcfour->set_encrypt_key(Crypto.SHA1->hash(secret));
  }

  string read(int size)
  {
    return arcfour->crypt(replace(allocate(size), 0, "\021") * "");
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
  Crypto.Sign key;

#ifdef ECDSA_MODE
#if constant(Crypto.ECC.Curve)
  key = Crypto.ECC.SECP_521R1.ECDSA()->
    set_random(Crypto.Random.random_string)->generate_key();
  my_certificate =
    Standards.X509.make_selfsigned_certificate(key, 3600*4, ([
						 "organizationName" : "Test",
						 "commonName" : "*",
					       ]));
  ecdsa_mode();
#else
#error ECDSA not supported by this Pike.
  exit(1);
#endif
#else
  SSL3_DEBUG_MSG("Cert: '%s'\n", String.string2hex(my_certificate));
  SSL3_DEBUG_MSG("Key:  '%s'\n", String.string2hex(my_key));
#if 0
  array key = SSL.asn1.ber_decode(my_key)->get_asn1()[1];
  SSL3_DEBUG_MSG("Decoded key: %O\n", key);
  object n = key[1][1];
  object e = key[2][1];
  object d = key[3][1];
  object p = key[4][1];
  object q = key[5][1];

  werror("n =  %s\np =  %s\nq =  %s\npq = %s\n",
	 n->digits(), p->digits(), q->digits(), (p*q)->digits());

  rsa = Crypto.RSA();
  rsa->set_public_key(n, e);
  rsa->set_private_key(d);
#else /* !0 */
  // FIXME: Is this correct?
  key = Standards.PKCS.RSA.parse_private_key(my_key);
#endif /* 0 */
  // Make sure all cipher suites are available.
  rsa_mode();
#endif
  SSL3_DEBUG_MSG("Cipher suites:\n%s", fmt_cipher_suites(preferred_suites));
  add_cert(key, ({ my_certificate }));
  random = no_random()->read;
  werror("Starting\n");
  if (!bind(PORT, my_accept_callback))
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
