#pike __REAL_VERSION__

import ".";

MySSLPort port;
int portno;
string|int(0..0) interface;
function(Request:void) callback;

program request_program=Request;

//! The simplest SSL server possible. Binds a port and calls
//! a callback with @[Request] objects.

//! Create a HTTPS (HTTP over SSL) server.
//!
//! @param _callback
//!   The function run when a request is received.
//!   takes one argument of type @[Request].
//! @param _portno
//!   The port number to bind to, defaults to 443.
//! @param _interface
//!   The interface address to bind to.
//! @param key
//!   An optional SSL secret key, provided in binary format, such
//!   as that created by @[Standards.PKCS.RSA.private_key()].
//! @param certificate
//!   An optional SSL certificate, provided in binary format.
void create(function(Request:void) _callback,
	    void|int _portno,
	    void|string _interface, void|string key, void|string certificate)
{
   portno=_portno;
   if (!portno) portno=443; // default HTTPS port

   callback=_callback;
   interface=_interface;

   port=MySSLPort();
   port->set_default_keycert();
   if(key)
     port->set_key(key);
   if(certificate)
     port->set_certificate(certificate);

   if (!port->bind(portno,new_connection,interface))
      error("HTTP.Server.SSLPort: failed to bind port %s%d: %s\n",
	    interface?interface+":":"",
	    portno,strerror(port->errno()));
}

//! Closes the HTTP port.
void close()
{
   destruct(port);
   port=0;
}

void destroy() { close(); }

//! The port accept callback
static void new_connection()
{
   Stdio.File fd=port->accept();
   Request r=request_program();
   r->attach_fd(fd,this,callback);
}

//!
class MySSLPort
{
#pike __REAL_VERSION__

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

//!
void set_default_keycert()
{
  set_key(my_key);
  set_certificate(my_certificate);
}

//!
void set_key(string skey)
{
#if 0
    array key = SSL.asn1.ber_decode(skey)->get_asn1()[1];
    object n = key[1][1];
    object e = key[2][1];
    object d = key[3][1];
    object p = key[4][1];
    object q = key[5][1];

    rsa = Crypto.rsa();
    rsa->set_public_key(n, e);
    rsa->set_private_key(d);
#else /* !0 */
    // FIXME: Is this correct?
    rsa = Standards.PKCS.RSA.parse_private_key(skey);
#endif /* 0 */

  }

//!
void set_certificate(string certificate)
{
    certificates = ({ certificate });
}

  void create()
  {
    sslport::create();
    random = Crypto.randomness.arcfour_random(
       sprintf("%s%4c", "Foo!", time()))->read;
  }

}
