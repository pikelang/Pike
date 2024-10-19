#pike __REAL_VERSION__
#require constant(SSL.Port)

inherit SSL.Port;

import ".";

int portno;
string interface;
function(Request:void) callback;

constant default_port = 443;
constant default_rsa_bits = 4096;
constant default_dsa_p_bits = 4096;
constant default_dsa_q_bits = 160;

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
//! @param reuse_port
//!   If true, enable SO_REUSEPORT if the OS supports it. See
//!   @[Stdio.Port.bind] for more information
protected void create(function(Request:void) callback,
                      int(1..) port = default_port,
                      void|string interface,
                      void|string|Crypto.Sign.State key,
                      void|string|array(string) certificate,
                      void|int reuse_port)
{
  ::create();

  portno = port;
  this::callback=callback;
  this::interface=interface;

  // NB: Bind the port first in case it fails.
  //     This is in order to not have to do set_default_keycert()
  //     multiple times.
  if (!Port::socket::bind(portno, UNDEFINED, this::interface, reuse_port))
    error("Failed to bind port %s%d: %s\n",
          interface?interface+":":"", portno, strerror(errno()));

  if( key && certificate )
  {
    if( stringp(certificate) )
      certificate = ({ certificate });
    ctx->add_cert( key, certificate, ({"*"}) );
  }
  else
    set_default_keycert();

  set_accept_callback(new_connection);
}

protected void _destruct() { close(); }

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
#ifdef WEBSOCKET_DEBUG
    werror("Generating %s private key and certificate...\n",
           private_key->name());
    int t = time();
#endif
    switch(private_key->name()) {
    case "RSA":
      private_key->generate_key(default_rsa_bits);
      break;
    case "DSA":
      private_key->generate_key(default_dsa_p_bits, default_dsa_q_bits);
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
#ifdef WEBSOCKET_DEBUG
    werror("Took %d seconds.\n", time() - t);
#endif
  }
}

protected string _sprintf(int t) {
  return t=='O' && sprintf("%O(%O:%d)", this_program, interface, portno);
}
