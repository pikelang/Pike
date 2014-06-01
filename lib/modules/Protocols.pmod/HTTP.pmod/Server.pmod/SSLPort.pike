#pike __REAL_VERSION__
#require constant(SSL.Cipher)

import ".";

MySSLPort port;
int portno;
string|int(0..0) interface;
function(Request:void) callback;

//!
object|function|program request_program=Request;

//! The simplest SSL server possible. Binds a port and calls
//! a callback with @[request_program] objects.

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
//!   An optional SSL certificate or chain of certificates with the host 
//!   certificate first, provided in binary format.
void create(function(Request:void) _callback,
	    void|int _portno,
	    void|string _interface, void|string key,
            void|string|array(string) certificate)
{
   portno=_portno;
   if (!portno) portno=443; // default HTTPS port

   callback=_callback;
   interface=_interface;

   port=MySSLPort();
   port->set_default_keycert();
   if( key && certificate )
     port->add_cert( key, certificate );

   if (!port->bind(portno,new_connection,[string]interface))
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
protected void new_connection()
{
   SSL.File fd=port->accept();
   Request r=request_program();
   r->attach_fd(fd,this,callback);
}

//!
class MySSLPort
{

  inherit SSL.Port;

  //!
  void set_default_keycert()
  {
    foreach(({ Crypto.RSA(), Crypto.DSA(),
#if constant(Crypto.ECC.Curve)
	       Crypto.ECC.SECP_521R1.ECDSA(),
#endif
	    }), Crypto.Sign private_key) {
      private_key->set_random(Crypto.Random.random_string);
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


  // ---- Remove this?

  private Crypto.Sign tmp_key;
  private array(string) tmp_cert;

  //! @deprecated add_cert
  __deprecated__ void set_key(string skey)
  {
    tmp_key = Standards.PKCS.RSA.parse_private_key(skey) ||
      Standards.PKCS.DSA.parse_private_key(skey) ||
#if constant(Crypto.ECC.Curve)
      Standards.PKCS.ECDSA.parse_private_key(skey) ||
#endif
      0;
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
}

protected string _sprintf(int t) {
  return t=='O' && sprintf("%O(%O:%d)", this_program, interface, portno);
}
