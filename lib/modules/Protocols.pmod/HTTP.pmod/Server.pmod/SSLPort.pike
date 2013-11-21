#pike __REAL_VERSION__

#if constant(SSL.Cipher.CipherAlgorithm)

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
	    void|string _interface, void|string key, void|string|array certificate)
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
   SSL.sslfile fd=port->accept();
   Request r=request_program();
   r->attach_fd(fd,this,callback);
}

//!
class MySSLPort
{

  inherit SSL.sslport;

  //!
  void set_default_keycert()
  {
    rsa = Crypto.RSA();
    rsa->generate_key( 4096 );

    mapping attrs = ([
      "organizationName" : "Pike TLS server",
      "commonName" : "*",
    ]);

    certificates = ({
      Standards.X509.make_selfsigned_certificate(rsa, 3600*24*365, attrs)
    });
  }

  //!
  void set_key(string skey)
  {
    rsa = Standards.PKCS.RSA.parse_private_key(skey);
  }

  //!
  void set_certificate(string|array(string) certificate)
  {
    if(arrayp(certificate))
      certificates = [array(string)]certificate;
    else
      certificates = ({ [string]certificate });
  }

  void create()
  {
    sslport::create();
    random = Crypto.Random.random_string;
  }

}

string _sprintf(int t) {
  return t=='O' && sprintf("%O(%O:%d)", this_program, interface, portno);
}

#endif
