#pike __REAL_VERSION__

/* $Id: https.pike,v 1.18 2004/04/19 22:59:06 nilsson Exp $
 *
 * dummy https server
 */

//! Dummy HTTPS server

#define PORT 25678

#if constant(SSL.Cipher.CipherAlgorithm)

import Stdio;

inherit SSL.sslport;

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

  string message = "<html><head><title>SSL-3 server</title></head>\n"
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
#ifdef SSL3_DEBUG
    werror("Received: '" + data + "'\n");
#endif
    sslfile->set_write_callback(write_callback);
  }

  void create(object f)
  {
    sslfile = f;
    sslfile->set_nonblocking(read_callback, 0, 0);
  }
}

class no_random {
  object arcfour = Crypto.Arcfour();
  
  void create(string|void secret)
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

void my_accept_callback(object f)
{
  werror("Accept!\n");
  conn(accept());
}

int main()
{
#ifdef SSL3_DEBUG
  werror("Cert: '%s'\n", Crypto.string_to_hex(my_certificate));
  werror("Key:  '%s'\n", Crypto.string_to_hex(my_key));
//  werror("Decoded cert: %O\n", SSL.asn1.ber_decode(my_certificate)->get_asn1());
#endif
#if 0
  array key = SSL.asn1.ber_decode(my_key)->get_asn1()[1];
#ifdef SSL3_DEBUG
  werror("Decoded key: %O\n", key);
#endif
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
  rsa = Standards.PKCS.RSA.parse_private_key(my_key);
#endif /* 0 */
  certificates = ({ my_certificate });
  random = no_random()->read;
  werror("Starting\n");
  if (!bind(PORT, my_accept_callback))
  {
    perror("");
    return 17;
  }
  else
    return -17;
}

void create()
{
#ifdef SSL3_DEBUG
  werror("https->create\n");
#endif
  sslport::create();
}

#endif
