#pike 7.9
#require constant(SSL.Cipher)

//! Dummy HTTPS server - Compat with Pike 7.8

#ifndef PORT
#define PORT 25678
#endif

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

import Stdio;

inherit 7.8::SSL.sslport;

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

  void create(object f)
  {
    sslfile = f;
    sslfile->set_nonblocking(read_callback, 0, 0);
  }
}

void my_accept_callback(object f)
{
  werror("Accept!\n");
  conn(accept());
}

int main()
{
  Crypto.Sign key;
  string certificate;

  key = Crypto.RSA()->generate_key(1024);
  certificate =
    Standards.X509.make_selfsigned_certificate(key, 3600*4, ([
						 "organizationName" : "Test",
						 "commonName" : "*",
					       ]));

  add_cert(key, ({ certificate }), ({ "*" }));

  key = Crypto.DSA()->generate_key(1024, 160);
  certificate =
    Standards.X509.make_selfsigned_certificate(key, 3600*4, ([
						 "organizationName" : "Test",
						 "commonName" : "*",
					       ]));
  add_cert(key, ({ certificate }));

#if constant(Crypto.ECC.Curve)
  key = Crypto.ECC.SECP_521R1.ECDSA()->generate_key();
  certificate =
    Standards.X509.make_selfsigned_certificate(key, 3600*4, ([
						 "organizationName" : "Test",
						 "commonName" : "*",
					       ]));
  add_cert(key, ({ certificate }));
#endif

  // Make sure all cipher suites are available.
  preferred_suites = get_suites(-1, 2);
  SSL3_DEBUG_MSG("Cipher suites:\n%s",
                 SSL.Constants.fmt_cipher_suites(preferred_suites));

  SSL3_DEBUG_MSG("Certs:\n%O\n", cert_pairs);

  random = Crypto.Random.random_string;
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
}

void create()
{
  SSL3_DEBUG_MSG("https->create\n");
  sslport::create();
}
