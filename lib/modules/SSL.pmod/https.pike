/* test_server.pike
 *
 * echoes packets over the SSL-packet layer
 */

#define PORT 25678

import Stdio;

inherit "sslport";

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

  void do_write()
  {
    if (index < strlen(message))
    {
      int written = sslfile->write(message[index..]);
      if (written > 0)
	index += written;
      else
	sslfile->close();
    }
    if (index == strlen(message))
      sslfile->close();
  }
  
  void read_callback(mixed id, string data)
  {
#ifdef SSL3_DEBUG
    werror("Recieved: '" + data + "'\n");
#endif
    do_write();
  }

  void write_callback(mixed id)
  {
    do_write();
  }

  void create(object f)
  {
    sslfile = f;
    sslfile->set_nonblocking(read_callback, write_callback, 0);
  }
}

class no_random {
  object rc4 = Crypto.rc4();
  
  void create(string|void secret)
  {
    if (!secret)
      secret = sprintf("Foo!%4c", time());
    object sha = Crypto.sha();
    sha->update(secret);
    rc4->set_encrypt_key(sha->digest());
  }

  string read(int size)
  {
    return rc4->crypt(replace(allocate(size), 0, "\021") * "");
  }
}

#if 0
/* ad-hoc asn.1-decoder */

class ber_decode {
  inherit ADT.struct;

  array get_asn1()
  {
    int tag = get_int(1);
    int len;
    string contents;
    
#ifdef SSL3_DEBUG
    werror(sprintf("decoding tag %x\n", tag));
#endif
    if ( (tag & 0x1f) == 0x1f)
      throw( ({ "high tag numbers is not supported\n", backtrace() }) );
    int len = get_int(1);
    if (len & 0x80)
      len = get_int(len & 0x7f);
    
#ifdef SSL3_DEBUG
    werror(sprintf("len : %d\n", len));
#endif

    contents = get_fix_string(len);
#ifdef SSL3_DEBUG
    werror(sprintf("contents: %O\n", contents));
#endif
    if (tag & 0x20)
    {
      object seq = object_program(this_object())(contents);
      array res = ({ });
      while(! seq->is_empty())
      {
	array elem = seq->get_asn1();
#ifdef SSL3_DEBUG
	// werror(sprintf("elem: %O\n", elem));
#endif
	res += ({ elem });
      }
      return ({ tag, res });
    }
    else
      return ({ tag, contents });
  }
}
#endif

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
  conn(f->accept());
}

int main()
{
#ifdef SSL3_DEBUG
  werror(sprintf("Cert: '%s'\n", Crypto.string_to_hex(my_certificate)));
  werror(sprintf("Key:  '%s'\n", Crypto.string_to_hex(my_key)));
//  werror(sprintf("Decoded cert: %O\n", SSL.asn1.ber_decode(my_certificate)->get_asn1()));
#endif
  array key = SSL.asn1.ber_decode(my_key)->get_asn1()[1];
#ifdef SSL3_DEBUG
  werror(sprintf("Decoded key: %O\n", key));
#endif
  object n = key[1][1];
  object e = key[2][1];
  object d = key[3][1];
  object p = key[4][1];
  object q = key[5][1];

  werror(sprintf("n =  %s\np =  %s\nq =  %s\npq = %s\n",
		 n->digits(), p->digits(), q->digits(), (p*q)->digits()));

  rsa = Crypto.rsa();
  rsa->set_public_key(n, e);
  rsa->set_private_key(d);
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
#ifdef SSL3_DEBUG
  sslport::create();
}
