#pike __REAL_VERSION__
#require constant(SSL.Port)

inherit SSL.Port;

import ".";

int portno;
string interface;
function(Request:void) callback;

//!
object|function|program request_program=Request;

//! A very simple SSL server. Binds a port and calls a callback with
//! @[request_program] objects.

//! Create a HTTPS (HTTP over SSL) server.
//!
//! @param callback
//!   The function run when a request is received.
//!   takes one argument of type @[Request].
//! @param port
//!   The port number to bind to, defaults to 443.
//! @param interface
//!   The interface address to bind to.
//! @param key
//!   An optional SSL secret key, provided in binary format, such
//!   as that created by @[Standards.PKCS.RSA.private_key()].
//! @param certificate
//!   An optional SSL certificate or chain of certificates with the host 
//!   certificate first, provided in binary format.
//! @param share
//!   If true, the connection will be shared if possible. See
//!   @[Stdio.Port.bind] for more information
protected void create(function(Request:void) callback,
                      void|int port,
                      void|string interface,
                      void|string|Crypto.Sign.State key,
                      void|string|array(string) certificate,
                      void|int share)
{
  ::create();

  portno = port || 443;
  this::callback=callback;
  this::interface=interface;

  if( key && certificate )
  {
    if( stringp(certificate) )
      certificate = ({ certificate });
    ctx->add_cert( key, certificate, ({"*"}) );
  }
  else
    set_default_keycert();

  if (!bind(portno, new_connection, this::interface, share))
    error("Failed to bind port %s%d: %s\n",
          interface?interface+":":"", portno, strerror(errno()));
}

void destroy() { close(); }

//! The port accept callback
protected void new_connection()
{
   SSL.File fd=accept();
   Request r=request_program();
   r->attach_fd(fd,this,callback);
}

protected void set_default_keycert()
{
  foreach(({ Crypto.RSA(), Crypto.DSA(),
#if constant(Crypto.ECC.Curve)
             Crypto.ECC.SECP_521R1.ECDSA(),
#endif
          }), Crypto.Sign private_key)
  {
    switch(private_key->name()) {
    case "RSA":
      private_key->generate_key(4096);
      break;
    case "DSA":
      private_key->generate_key(4096, 160);
      break;
    default:
      // ECDSA.
      private_key->generate_key();
      break;
    }

    mapping a = ([
      "organizationName" : "Pike TLS server",
      "commonName" : "*",
    ]);

    string c = Standards.X509.make_selfsigned_certificate(private_key,
                                                          3600*24*365, a);

    ctx->add_cert( private_key, ({ c }) );
  }
}

protected string _sprintf(int t) {
  return t=='O' && sprintf("%O(%O:%d)", this_program, interface, portno);
}

// ---- Deprecated

private string tmp_key;
private array(string) tmp_cert;

__deprecated__ this_program `port()
{
  return this;
}

//! @deprecated add_cert
__deprecated__ void set_key(string skey)
{
  tmp_key = skey;
  if( tmp_key && tmp_cert )
    ctx->add_cert( tmp_key, tmp_cert );
}

//! @deprecated add_cert
__deprecated__ void set_certificate(string|array(string) certificate)
{
  if(arrayp(certificate))
    tmp_cert = [array(string)]certificate;
  else
    tmp_cert = ({ [string]certificate });
  if( tmp_key && tmp_cert )
    ctx->add_cert( tmp_key, tmp_cert );
}
